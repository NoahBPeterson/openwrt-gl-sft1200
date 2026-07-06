// gl-ucode object: dpi  (port of decompiled-spec/rpc/dpi.lua, 4.9.0)
// App-aware QoS / content-protection / DPI stats. Backed by GL's netifyd + nDPI
// stack, which is NOT installed on the Siflower SFT1200 test device.
//
// This port keeps GL's uci as its OWN config store so set/get round-trip
// faithfully:  gl_dpi, gl_dpi_qos, gl_dpi_content_protection,
// gl_dpi_flow_statistics.  Getters return sane DISABLED defaults; the set_*
// methods persist config and, ONLY when netifyd is actually present
// (fs.access("/etc/init.d/netifyd")), also rebuild the netifyd conf snippets,
// the fw4 nft rules and kick the daemon.
//
// DEGRADE / dropped GL sources (absent on this device):
//   - the whole `gl-dpi` ubus daemon (get_dpi_status / version / ota_update /
//     set_lib_upgrade / enable_dpi_base_service / set_auto_upgrade). Every call
//     is treated as "unavailable" -> disabled/no-op result.
//   - the GL cloud licence flow (/cloud-api/.../getLicence, firmware-api OTA
//     check). We treat the router as ALWAYS UNLICENSED / DISABLED: check_* say
//     no upgrade available, get_subscribe_popup -> {enable:false},
//     enable_dpi_base_service fails (no licence), confirm is a no-op.
//   - lsqlite3 traffic DB (/tmp/traffic_data.db): get_dpi_stats has no engine to
//     read, so traffic_day is 0 and the qos / content_protection counters come
//     from the /tmp stat files our set_* methods write.
//   - gl-session cross-module calls (network get/set_netnat_config, flow_statistics
//     set_statistics_rule) and the MediaTek/iptables (non-fw4) branches: dropped.
//   - `gl_eqos` reload: dropped (GL C qos engine, absent).
// PRESERVED: all 18 method names and every app-facing JSON field name/shape.
'use strict';

const fs = require('fs');

const APP_METADATA = "/etc/netifyd/app-metadata.json";
const NFA_QOS_DIR = "/etc/netifyd/nfa-qos.d";
const CP_BLOCK = "/etc/netifyd/nfa-block.d/10-app-content-protection.conf";
const CP_DOMAIN = "/etc/netifyd/categories.d/10-gl_content_protection.conf";
const LIB_VERSION_DEFAULT = "20251218_00";

// netifyd (the DPI engine) present? Everything network-affecting is gated on this.
function have_netifyd() { return fs.access("/etc/init.d/netifyd"); }
function have_fw4() { return fs.access("/sbin/fw4"); }

// uci list options come back as an array; a single value as a string; missing as
// null. Normalise to an array.
function as_list(v) {
	if (type(v) == "array") return v;
	if (type(v) == "string" && length(v) > 0) return [v];
	return [];
}

// create a uci section if it does not exist (these configs don't ship on mainline)
function ensure(c, cfg, sec, typ) {
	if (!c.get(cfg, sec)) c.set(cfg, sec, typ);
}

function today() {
	let d = trim(system_popen("date +%Y-%m-%d"));
	return d ? d : "";
}

// tiny shell helper for the couple of places the original shelled `date`
function system_popen(cmd) {
	let p = fs.popen(cmd);
	if (!p) return "";
	let out = p.read("all");
	p.close();
	return out ? trim(out) : "";
}

function any_dpi_enabled(c) {
	return c.get("gl_dpi_qos", "qos", "prio_enable") == "1"
	    || c.get("gl_dpi_content_protection", "content_protection", "enabled") == "1"
	    || c.get("gl_dpi_flow_statistics", "global", "enable") == "1";
}

// build a netifyd category-match snippet from GL's gl_category config (absent on
// mainline -> returns ""). Ported faithfully for the netifyd-present case.
function get_netifyd_categorys(c, glcat) {
	let out = "";
	for (let cat in as_list(c.get("gl_category", glcat, "category")))
		out += sprintf("category=='%s';\n", cat);
	for (let cat in as_list(c.get("gl_category", glcat, "protocol")))
		out += sprintf("protocol_category=='%s'&&category=='unclassified';\n", cat);
	return out;
}

function generate_category_rules(c, categories) {
	let out = "";
	for (let category in (type(categories) == "array" ? categories : []))
		out += get_netifyd_categorys(c, category);
	return out;
}

// DEGRADE: stop netifyd + tear down its fw4 table when nothing else needs it.
function stop_netifyd_process(ctx, c) {
	if (any_dpi_enabled(c)) return;
	ctx.popen("/etc/init.d/netifyd stop >/dev/null 2>&1");
	if (have_fw4())
		ctx.popen("nft flush table inet netify 2>/dev/null");
}

// DEGRADE: (re)install the netifyd fw4 forward rules + restart, when needed.
function start_netifyd_process(ctx, c) {
	if (!any_dpi_enabled(c)) return;
	if (have_fw4()) {
		ctx.popen("nft add rule inet netify forward 'ct label set netify-init counter' 2>/dev/null");
		ctx.popen("nft add rule inet netify forward 'ct packets > 32 counter return' 2>/dev/null");
		ctx.popen("nft add rule inet netify forward 'counter queue num 0-1 fanout, bypass' 2>/dev/null");
	}
	ctx.popen("/etc/init.d/netifyd restart >/dev/null 2>&1");
}

// resolve a netifyd category rule snippet -> map of {name,label,icon} using the
// netifyd metadata files. Only meaningful when netifyd is installed.
function app_meta(name) {
	// DEGRADE: without app-metadata.json we can only echo the raw app name.
	return { name: name, label: name, icon: "" };
}

export default {
	// ---- QoS -------------------------------------------------------------
	set_qos: function(args, ctx) {
		let c = ctx.uci.cursor();
		if (type(args.enable) != "bool")
			return ctx.util.E.INVALID_PARAMS;
		if (type(args.upload) != "int" && type(args.upload) != "double" && type(args.upload) != "string")
			return ctx.util.E.INVALID_PARAMS;
		if (type(args.download) != "int" && type(args.download) != "double" && type(args.download) != "string")
			return ctx.util.E.INVALID_PARAMS;
		let upload = +args.upload;
		let download = +args.download;
		if (!(upload >= 1 && upload <= 10000))
			return ctx.util.E.INVALID_PARAMS;
		if (!(download >= 1 && download <= 10000))
			return ctx.util.E.INVALID_PARAMS;

		// DEGRADE: sqm "please turn off sqm" guard, gl-session netnat disable and
		// `gl_eqos` uci `qos` teardown/reload dropped (GL/MediaTek-only; QoS here
		// is store-only since the netifyd shaper is absent).

		// ---- config store (always persisted so the app round-trips) ----
		ensure(c, "gl_dpi_qos", "qos", "dpi");
		c.set("gl_dpi_qos", "qos", "upload", int(upload));
		c.set("gl_dpi_qos", "qos", "download", int(download));
		c.set("gl_dpi_qos", "qos", "prio_enable", args.enable ? 1 : 0);

		let priority_categories = {
			"1": "work_meeting",
			"2": "game",
			"3": "webpage",
			"4": "social_entertainment"
		};

		if (args.mode != null && args.enable) {
			c.set("gl_dpi_qos", "qos", "prio_mode", args.mode);
			if (args.mode == "5") {
				ensure(c, "gl_dpi_qos", "priority_high", "dpi");
				ensure(c, "gl_dpi_qos", "priority_middle", "dpi");
				ensure(c, "gl_dpi_qos", "priority_low", "dpi");
				c.delete("gl_dpi_qos", "priority_high", "category");
				if (type(args.high) == "array" && length(args.high) > 0)
					c.set("gl_dpi_qos", "priority_high", "category", args.high);
				c.delete("gl_dpi_qos", "priority_middle", "category");
				if (type(args.middle) == "array" && length(args.middle) > 0)
					c.set("gl_dpi_qos", "priority_middle", "category", args.middle);
				c.delete("gl_dpi_qos", "priority_low", "category");
				if (type(args.low) == "array" && length(args.low) > 0)
					c.set("gl_dpi_qos", "priority_low", "category", args.low);
			}
		}
		c.commit("gl_dpi_qos");
		ctx.popen("sync");

		// ---- netifyd enforcement (only when the engine is present) ----
		if (have_netifyd()) {
			if (!args.enable) {
				fs.writefile(`${NFA_QOS_DIR}/10-app-qos-high.conf`, "");
				fs.writefile(`${NFA_QOS_DIR}/10-app-qos-middle.conf`, "");
				fs.writefile(`${NFA_QOS_DIR}/10-app-qos-low.conf`, "");
			}
			if (args.mode != null && args.enable) {
				if (args.mode != "5") {
					let category = priority_categories[args.mode];
					let result = "";
					if (category == "work_meeting")
						result = get_netifyd_categorys(c, "work") + get_netifyd_categorys(c, "meeting");
					else if (category != null)
						result = get_netifyd_categorys(c, category);
					fs.writefile(`${NFA_QOS_DIR}/10-app-qos-high.conf`, result);
					fs.writefile(`${NFA_QOS_DIR}/10-app-qos-middle.conf`, "");
					fs.writefile(`${NFA_QOS_DIR}/10-app-qos-low.conf`, "");
				} else {
					fs.writefile(`${NFA_QOS_DIR}/10-app-qos-high.conf`, generate_category_rules(c, args.high));
					fs.writefile(`${NFA_QOS_DIR}/10-app-qos-middle.conf`, generate_category_rules(c, args.middle));
					fs.writefile(`${NFA_QOS_DIR}/10-app-qos-low.conf`, generate_category_rules(c, args.low));
				}
			}
			if (args.enable) {
				ctx.popen("/usr/bin/gl_dpi_qos start >/dev/null 2>&1");
				start_netifyd_process(ctx, c);
			} else {
				ctx.popen("/usr/bin/gl_dpi_qos stop >/dev/null 2>&1");
				stop_netifyd_process(ctx, c);
			}
		}

		if (!args.enable) {
			// keep the stat file our get_dpi_stats reads in sync
			let qos_content = sprintf("qos:0 date:%s", today());
			fs.writefile("/tmp/qos_stats", qos_content);
			if (have_netifyd())
				fs.writefile("/etc/netifyd/qos_stats", qos_content);
		}
		return {};
	},

	get_qos: function(args, ctx) {
		let c = ctx.uci.cursor();
		let upload = c.get("gl_dpi_qos", "qos", "upload");
		let download = c.get("gl_dpi_qos", "qos", "download");
		return {
			mode: c.get("gl_dpi_qos", "qos", "prio_mode") ?? "",
			enable: (c.get("gl_dpi_qos", "qos", "prio_enable") == "1"),
			upload: (upload == null || upload == "0") ? "" : upload,
			download: (download == null || download == "0") ? "" : download,
			high: as_list(c.get("gl_dpi_qos", "priority_high", "category")),
			middle: as_list(c.get("gl_dpi_qos", "priority_middle", "category")),
			low: as_list(c.get("gl_dpi_qos", "priority_low", "category"))
		};
	},

	// DEGRADE: the app catalog is built from netifyd's netify-apps.conf +
	// app-metadata.json + netify-categories.json + gl_category, none of which
	// exist without the engine -> return an empty catalog.
	get_apps: function(args, ctx) {
		if (!have_netifyd() || !fs.access(APP_METADATA))
			return [];
		// Engine present but the full catalog pipeline (metadata join across three
		// files) is out of scope for this degrade port; return empty rather than
		// a partial/incorrect list.
		return [];
	},

	// ---- Content protection ---------------------------------------------
	get_content_protection: function(args, ctx) {
		let c = ctx.uci.cursor();
		let res = {
			enable: (c.get("gl_dpi_content_protection", "content_protection", "enabled") == "1"),
			domain_list: as_list(c.get("gl_dpi_content_protection", "content_protection", "domain")),
			category_list: as_list(c.get("gl_dpi_content_protection", "content_protection", "category")),
			app_list: []
		};
		// DEGRADE: without netifyd metadata each app resolves to name/label=name/icon="".
		for (let name in as_list(c.get("gl_dpi_content_protection", "content_protection", "app")))
			push(res.app_list, app_meta(name));
		return res;
	},

	set_content_protection: function(args, ctx) {
		let c = ctx.uci.cursor();
		if (args.domain_list != null && type(args.domain_list) == "array" && length(args.domain_list) > 10000)
			return ctx.util.E.INVALID_PARAMS;

		// clean the custom domain list (drop blank / whitespace-only entries)
		let domain_list = [];
		for (let domain in (type(args.domain_list) == "array" ? args.domain_list : [])) {
			let s = `${domain}`;
			if (match(s, /\S/)) push(domain_list, domain);
		}

		let enable_old = (c.get("gl_dpi_content_protection", "content_protection", "enabled") == "1");

		// DEGRADE: gl-session netnat disable dropped.

		// ---- netifyd conf snippets (only when the engine is present) ----
		if (have_netifyd()) {
			let block = "";
			let dom = "";
			if (args.enable) {
				block = "category=='gl_content_protection';\n";
				for (let category in as_list(args.category_list))
					if (category != "") block += sprintf("category=='%s';\n", category);
				for (let app in as_list(args.app_list))
					if (app != "") block += sprintf("app=='%s';\n", app);
				for (let domain in domain_list) {
					// DEGRADE: gl.validator.ipaddr absent -> lightweight ip vs domain test.
					if (match(`${domain}`, /^[0-9.:\/]+$/))
						dom += sprintf("net:%s\n", domain);
					else
						dom += sprintf("dom:%s\n", domain);
				}
			}
			fs.writefile(CP_BLOCK, block);
			fs.writefile(CP_DOMAIN, dom);
		}

		// ---- config store (always persisted so the app round-trips) ----
		ensure(c, "gl_dpi_content_protection", "content_protection", "content_protection");
		c.delete("gl_dpi_content_protection", "content_protection", "category");
		c.delete("gl_dpi_content_protection", "content_protection", "app");
		c.delete("gl_dpi_content_protection", "content_protection", "domain");
		if (type(args.category_list) == "array" && length(args.category_list) > 0)
			c.set("gl_dpi_content_protection", "content_protection", "category", args.category_list);
		if (type(args.app_list) == "array" && length(args.app_list) > 0)
			c.set("gl_dpi_content_protection", "content_protection", "app", args.app_list);
		if (length(domain_list) > 0)
			c.set("gl_dpi_content_protection", "content_protection", "domain", domain_list);
		if (args.enable != null)
			c.set("gl_dpi_content_protection", "content_protection", "enabled", args.enable ? "1" : "0");
		c.commit("gl_dpi_content_protection");
		ctx.popen("sync");

		if (!args.enable) {
			let content = sprintf("content_protection_day:0 content_protection_all:0 date:%s", today());
			fs.writefile("/tmp/content_protection_stats", content);
			if (have_netifyd()) {
				fs.writefile("/etc/netifyd/content_protection_stats", content);
				stop_netifyd_process(ctx, c);
				if (have_fw4())
					ctx.popen("nft flush chain inet filter parental_control 2>/dev/null");
			}
		}

		if (have_netifyd()) {
			if (args.enable != enable_old && args.enable) {
				if (have_fw4()) {
					ctx.popen("nft add rule inet filter parental_control 'ip daddr @GL_DPI_BLOCK counter name \"block\" drop' 2>/dev/null");
					ctx.popen("nft add rule inet filter parental_control 'ct label \"netify-allow\" counter accept' 2>/dev/null");
					ctx.popen("nft add rule inet filter parental_control 'ct label \"netify-block\" counter name \"block\" drop' 2>/dev/null");
				}
				let qos_enable = (c.get("gl_dpi_qos", "qos", "prio_enable") == "1");
				let flow_enable = (c.get("gl_dpi_flow_statistics", "global", "enable") == "1");
				if (!qos_enable && !flow_enable)
					start_netifyd_process(ctx, c);
				else
					ctx.popen("/etc/init.d/netifyd reload >/dev/null 2>&1");
			} else {
				ctx.popen("/etc/init.d/netifyd reload >/dev/null 2>&1");
			}
			ctx.popen("lua /etc/netifyd/dpi_block.lua >/dev/null 2>&1");
		}
		return {};
	},

	mod_app_content_protection: function(args, ctx) {
		let c = ctx.uci.cursor();
		if (args.app == null || args.action == null)
			return ctx.util.E.INVALID_PARAMS;
		if (c.get("gl_dpi_content_protection", "content_protection", "enabled") != "1")
			return ctx.util.E.INVALID_PARAMS;

		let app_list = as_list(c.get("gl_dpi_content_protection", "content_protection", "app"));
		let same_app = false;
		for (let app in app_list)
			if (app == args.app) same_app = true;

		if ((args.action == "add" && same_app) || (args.action == "del" && !same_app))
			return ctx.util.E.INVALID_PARAMS;

		c.delete("gl_dpi_content_protection", "content_protection", "app");
		if (args.action == "add") {
			push(app_list, args.app);
			c.set("gl_dpi_content_protection", "content_protection", "app", app_list);
		} else {
			let kept = [];
			for (let app in app_list)
				if (app != args.app) push(kept, app);
			app_list = kept;
			if (length(app_list) > 0)
				c.set("gl_dpi_content_protection", "content_protection", "app", app_list);
		}
		c.commit("gl_dpi_content_protection");
		ctx.popen("sync");

		if (have_netifyd()) {
			// rebuild the app block file from the current list
			let block = "";
			if (args.action == "del")
				block = get_netifyd_categorys(c, "dangerous_website");
			for (let app in app_list)
				if (app != "") block += sprintf("app=='%s';\n", app);
			fs.writefile(CP_BLOCK, block);
			ctx.popen("/etc/init.d/netifyd reload >/dev/null 2>&1");
			ctx.popen("lua /etc/netifyd/dpi_block.lua >/dev/null 2>&1");
		}
		return {};
	},

	// ---- DPI status / config --------------------------------------------
	// DEGRADE: gl-dpi ubus daemon absent -> report the engine disabled/unlicensed.
	get_dpi_status: function(args, ctx) {
		return {
			status: 0,
			base_status: false,
			license_status: 0,
			expire_time: 0,
			enable: false
		};
	},

	get_dpi_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		return {
			lib_update_auto: (c.get("gl_dpi", "dpi_config", "lib_auto_upgrade") == "1"),
			ota_update_auto: (c.get("gl_dpi", "dpi_config", "ota_auto_upgrade") == "1"),
			lib_version: c.get("gl_dpi", "dpi_config", "lib_version") ?? LIB_VERSION_DEFAULT,
			ota_version: ""   // DEGRADE: gl-dpi `version` ubus absent
		};
	},

	set_dpi_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		if (args.lib_update_auto != null && type(args.lib_update_auto) != "bool")
			return ctx.util.E.INVALID_PARAMS;
		if (args.ota_update_auto != null && type(args.ota_update_auto) != "bool")
			return ctx.util.E.INVALID_PARAMS;
		ensure(c, "gl_dpi", "dpi_config", "dpi");
		if (args.lib_update_auto != null)
			c.set("gl_dpi", "dpi_config", "lib_auto_upgrade", args.lib_update_auto ? "1" : "0");
		if (args.ota_update_auto != null)
			c.set("gl_dpi", "dpi_config", "ota_auto_upgrade", args.ota_update_auto ? "1" : "0");
		c.commit("gl_dpi");
		ctx.popen("sync");
		// DEGRADE: gl-dpi set_auto_upgrade / triggered set_lib_upgrade dropped.
		return {};
	},

	// ---- Upgrade / check (all DEGRADE to "nothing available/unsupported") ----
	// DEGRADE: cloud firmware-api / gl-dpi absent and router treated as unlicensed.
	check_dpi_upgrade: function(args, ctx) {
		return { upgrade_available: false, dpi_new_version: "" };
	},

	check_lib_upgrade: function(args, ctx) {
		return { upgrade_available: false, lib_new_version: "" };
	},

	// DEGRADE: no gl-dpi ota_update engine -> no-op "already up to date" (status 0).
	set_dpi_upgrade: function(args, ctx) {
		return { status: 0 };
	},

	// DEGRADE: no gl-dpi set_lib_upgrade engine -> no-op success.
	set_lib_upgrade: function(args, ctx) {
		return { status: 0 };
	},

	// DEGRADE: base service needs a valid cloud licence (always absent) -> fail.
	enable_dpi_base_service: function(args, ctx) {
		if (type(args.enable) != "bool")
			return ctx.util.E.INVALID_PARAMS;
		return { status: -1, msg: "unsupported" };
	},

	set_ignore_subuscribe: function(args, ctx) {
		if (type(args.enable) != "bool")
			return ctx.util.E.INVALID_PARAMS;
		fs.writefile("/etc/ignore_dpi_subscribe", args.enable ? "1" : "0");
		return {};
	},

	// DEGRADE: no licence -> no subscription expiry -> never pop the subscribe dialog.
	get_subscribe_popup: function(args, ctx) {
		return { enable: false };
	},

	// DEGRADE: gl-session cross-module confirm (content_protection / flow_statistics /
	// netnat) needs the engine + a licence -> no-op.
	set_dpi_base_service_confirm: function(args, ctx) {
		return {};
	},

	// DEGRADE: no lsqlite3 traffic DB -> traffic_day 0; the qos / content-protection
	// counters come from the /tmp stat files the set_* methods maintain.
	get_dpi_stats: function(args, ctx) {
		let res = { traffic_day: 0, content_protection_day: 0, content_protection_all: 0, qos: 0 };
		let cp = fs.readfile("/tmp/content_protection_stats") ?? "";
		let qos = fs.readfile("/tmp/qos_stats") ?? "";
		let m = match(cp, /content_protection_day:([0-9]+)/);
		if (m) res.content_protection_day = int(m[1]);
		m = match(cp, /content_protection_all:([0-9]+)/);
		if (m) res.content_protection_all = int(m[1]);
		m = match(qos, /qos:([0-9]+)/);
		if (m) res.qos = int(m[1]);
		return res;
	}
};
