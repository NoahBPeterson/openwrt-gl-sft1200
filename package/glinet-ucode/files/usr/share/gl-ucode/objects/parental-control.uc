// gl-ucode object: parental-control  (port of decompiled-spec/rpc/parental-control.lua)
// Per-group scheduling + domain/app filtering. The `parental_control_v2` uci config is
// kept as our OWN store (global / group / schedule / rule sections + per-rule
// blacklist.conf / apps.conf files), exactly as GL laid it out.
//
// TIER 1 (reimplemented over mainline):
//   - group MAC membership, time SCHEDULES (week/begin/end/days) and DOMAIN blacklists.
//   - Enforcement replaces GL's closed `/etc/init.d/parental_control` DPI daemon:
//       * MAC + time windows -> a self-managed `inet gl_parental` nftables table that
//         drops LAN-origin traffic from each group's MACs during its scheduled windows
//         (nft `meta day` / `meta hour`), rebuilt from the store on every set_*.
//       * domain blacklists -> a dnsmasq blocklist (`address=/domain/`) aggregated from
//         every ENABLED group into /tmp/dnsmasq.d/parental_control.conf + dnsmasq restart.
//   - The group/schedule/rule state machine and the blacklist validators (253-char
//     length cap + http(s):// URL rejection) are ported faithfully.
//
// TIER 2 (DEGRADE -- needs the closed netifyd app-signature DB):
//   - get_app_list and the apps/category filtering depend on /etc/apps.conf,
//     /etc/app-metadata.json, /etc/categories.json and the `parental_control_apps` uci
//     (all shipped by netifyd, ABSENT on mainline). If those files are missing:
//       * get_app_list -> []  (empty result)
//       * set_group / add_group STILL STORE `apps` / `category` (so the app round-trips
//         them via get_config) but they are NOT enforced -- nft/dnsmasq only act on
//         MAC/time/domain. If the files ARE present the full app-list logic runs.
//
// PRESERVED: every method name and every app-facing JSON field/shape.
'use strict';

const fs = require('fs');

const CONFIG = "parental_control_v2";
const RULE_DIR = "/etc/parental_control_v2";
const DNS_CONF = "/tmp/dnsmasq.d/parental_control.conf";
const MAX_GROUPS = 10;

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

// uci list options come back as an array; a single scalar as a string; missing as
// null. Normalise everything to an array.
function as_list(v) {
	if (type(v) == "array") return v;
	if (type(v) == "string" && length(v) > 0) return [v];
	return [];
}

// rand()/srand() aren't globals in the uhttpd ucode VM; derive a candidate from a
// time-seeded LCG and walk until we find an unused "<prefix>NNNNNNN" section (the
// original used math.random(1000000,9999999)).
function random_uci_section(c, prefix) {
	let seed = time();
	for (let i = 0; i < 1000; i++) {
		let n = ((seed * 1103515245 + i * 2654435761 + 12345) % 9000000);
		if (n < 0) n = -n;
		n += 1000000;
		let sec = prefix + n;
		if (!c.get(CONFIG, sec)) return sec;
	}
	return null;
}

// { long: <any entry longer than 253 chars>, arr: [the offending entries] }
function get_long_list(list) {
	let arr = [], long = false;
	for (let s in list) {
		if (type(s) == "string" && length(s) > 253) { push(arr, s); long = true; }
	}
	return { long: long, arr: arr };
}

// { has: <any entry was an http(s):// URL>, arr: [entries with the scheme stripped
// to their host] }. Mirrors the Lua handle_url(): reject blacklist entries that were
// passed as full URLs, returning the host part so the app can re-submit clean domains.
function handle_url(list) {
	let arr = [], has = false;
	for (let s in list) {
		if (type(s) != "string") { push(arr, s); continue; }
		if (substr(s, 0, 7) == "http://") {
			let rest = substr(s, 7);
			let m = match(rest, /^([0-9a-zA-Z.:-]+)\//);
			push(arr, m ? m[1] : rest);
			has = true;
		} else if (substr(s, 0, 8) == "https://") {
			let rest = substr(s, 8);
			let m = match(rest, /^([0-9a-zA-Z.:-]+)\//);
			push(arr, m ? m[1] : rest);
			has = true;
		} else {
			push(arr, s);
		}
	}
	return { has: has, arr: arr };
}

// nft set/identifier-safe version of a uci section id.
function nft_id(sid) {
	return "g_" + replace(sid, /[^0-9a-zA-Z_]/g, "_");
}

// parse a schedule "week" (uci scalar "1,3,5", a bare day number, or a uci list) into
// a list of nft day numbers (0=Sunday..6=Saturday). null if nothing usable.
function parse_days(week) {
	let toks = [];
	if (type(week) == "array") toks = week;
	else if (type(week) == "string" && week != "") toks = split(week, /[^0-9]+/);
	else return null;
	let days = [];
	for (let t in toks) {
		if (type(t) != "string") t = "" + t;
		if (t == "") continue;
		let n = int(t);
		if (n >= 0 && n <= 6) push(days, n);
	}
	return length(days) ? days : null;
}

function valid_hhmm(s) {
	return (type(s) == "string" && match(s, /^[0-9]{1,2}:[0-9]{2}$/)) ? true : false;
}

// read a per-rule conf file (blacklist.conf / apps.conf) into a list of non-empty lines.
function read_lines(path) {
	let out = [];
	let raw = fs.readfile(path);
	if (type(raw) != "string") return out;
	for (let line in split(raw, /[\r\n]+/))
		if (line != "") push(out, line);
	return out;
}

// ---------------------------------------------------------------------------
// enforcement -- replaces GL's closed "/etc/init.d/parental_control restart"
// ---------------------------------------------------------------------------

// Rebuild the `inet gl_parental` nft table (MAC + time-window drops) and the dnsmasq
// domain blocklist from the current uci store. Idempotent; safe to call on every write.
function apply(ctx) {
	let c = ctx.uci.cursor();
	let global_enable = c.get(CONFIG, "global", "enable");

	// index groups + schedules ------------------------------------------------
	let groups = {};
	c.foreach(CONFIG, "group", function(s) {
		groups[s[".name"]] = {
			enabled: s.enabled != "0",
			sched_enabled: s.schedules_enabled != "0",
			macs: as_list(s.macs),
			rule: s.default_rule
		};
	});
	let schedules = [];
	c.foreach(CONFIG, "schedule", function(s) {
		push(schedules, {
			group: s.group,
			enabled: s.enabled != "0",
			week: s.week,
			begin: s.begin,
			end: s["end"]
		});
	});

	// nftables MAC/time drops -------------------------------------------------
	// always start from a clean slate (delete-if-exists without aborting)
	ctx.popen("nft add table inet gl_parental 2>/dev/null; nft delete table inet gl_parental 2>/dev/null");

	if (global_enable != "0") {
		let sets = "", rulelines = "";
		for (let gid in groups) {
			let g = groups[gid];
			if (!g.enabled || !g.sched_enabled) continue;
			let clean = [];
			for (let m in g.macs)
				if (type(m) == "string" && match(m, /^[0-9a-fA-F]{2}(:[0-9a-fA-F]{2}){5}$/))
					push(clean, lc(m));
			if (length(clean) == 0) continue;

			// this group's ENABLED schedule windows
			let wins = [];
			for (let sch in schedules)
				if (sch.group == gid && sch.enabled) push(wins, sch);
			if (length(wins) == 0) continue;

			let setname = nft_id(gid);
			sets += "\tset " + setname + " {\n" +
			        "\t\ttype ether_addr\n" +
			        "\t\telements = { " + join(", ", clean) + " }\n" +
			        "\t}\n";
			for (let w in wins) {
				let clause = "\t\tether saddr @" + setname;
				let days = parse_days(w.week);
				if (days) clause += " meta day { " + join(", ", days) + " }";
				if (valid_hhmm(w.begin) && valid_hhmm(w.end))
					clause += " meta hour \"" + w.begin + "\"-\"" + w.end + "\"";
				clause += " drop\n";
				rulelines += clause;
			}
		}
		if (rulelines != "") {
			let ruleset =
				"table inet gl_parental {\n" +
				sets +
				"\tchain fwd {\n" +
				"\t\ttype filter hook forward priority filter; policy accept;\n" +
				rulelines +
				"\t}\n" +
				"\tchain inp {\n" +
				"\t\ttype filter hook input priority filter; policy accept;\n" +
				rulelines +
				"\t}\n" +
				"}\n";
			fs.writefile("/tmp/gl-parental.nft", ruleset);
			ctx.popen("nft -f /tmp/gl-parental.nft 2>/dev/null");
		}
	}

	// dnsmasq domain blocklist ------------------------------------------------
	// DEGRADE: GL's DPI daemon filtered domains PER-MAC/per-schedule. dnsmasq can only
	// block globally, so we aggregate every ENABLED group's blacklist into one conf
	// (no per-MAC / per-time scoping). OpenWrt's dnsmasq init auto-includes
	// /tmp/dnsmasq.d/*.conf.
	let domains = {};
	if (global_enable != "0") {
		for (let gid in groups) {
			let g = groups[gid];
			if (!g.enabled || !g.rule) continue;
			for (let d in read_lines(RULE_DIR + "/" + g.rule + "blacklist.conf"))
				domains[d] = true;
		}
	}
	let lines = [];
	for (let d in domains) push(lines, "address=/" + d + "/");
	if (length(lines) > 0) {
		ctx.popen("mkdir -p /tmp/dnsmasq.d");
		fs.writefile(DNS_CONF, join("\n", lines) + "\n");
	} else {
		ctx.popen("rm -f " + DNS_CONF);
	}
	ctx.popen("/etc/init.d/dnsmasq restart >/dev/null 2>&1");
}

// ---------------------------------------------------------------------------
// TIER 2 (netify) -- get_app_list
// ---------------------------------------------------------------------------

function build_app_list(ctx) {
	// DEGRADE: all four inputs ship with netifyd and are absent on mainline.
	let conf_content = fs.readfile("/etc/apps.conf");
	if (type(conf_content) != "string") return [];

	let metaraw = fs.readfile("/etc/app-metadata.json");
	if (type(metaraw) != "string") return [];
	let mdata = null;
	try { mdata = json(metaraw); } catch (e) { return []; }
	if (type(mdata) != "array") return [];

	// metadata_index[id] = { label, icon }
	let metadata_index = {};
	for (let item in mdata) {
		if (item != null && item.id != null) {
			let icon = item.icon ? replace("" + item.icon, "\\/", "/") : "Unknown";
			metadata_index["" + int(item.id)] = {
				label: (item.label != null) ? item.label : "Unknown",
				icon: icon
			};
		}
	}

	// apps[id] = { name, label, icon }
	let apps = {};
	for (let line in split(conf_content, /[\r\n]+/)) {
		let m = match(line, /app:([0-9]+):([0-9a-zA-Z._-]+)/);
		if (!m) continue;
		let id = m[1];
		let name = replace(m[2], /^netify\./, "");
		let md = metadata_index["" + int(id)];
		apps[id] = {
			name: name,
			label: (md && md.label != null) ? md.label : "Unknown",
			icon: (md && md.icon != null) ? md.icon : "Unknown"
		};
	}

	let json_content = fs.readfile("/etc/categories.json");
	if (type(json_content) != "string") return [];
	let data = null;
	try { data = json(json_content); } catch (e) { return []; }
	if (type(data) != "object") return [];

	let c = ctx.uci.cursor();
	let result = [];
	let big_categories = {};
	c.foreach("parental_control_apps", "category", function(s) {
		if (!s.category) return;
		let big_name = s[".name"];
		big_categories[big_name] ??= {};
		for (let small_cat in as_list(s.category)) {
			if (big_name == "sexual_content" || big_name == "gambling" || big_name == "malicious_content")
				push(result, { category_name: big_name, apps: [] });
			big_categories[big_name][small_cat] = true;
		}
	});

	let small_category_to_apps = {};
	let category_tag_index = (data.application_tag_index != null) ? data.application_tag_index : {};
	let application_index = (data.application_index != null) ? data.application_index : [];
	for (let item in application_index) {
		let cat_id = "" + item[0];
		let app_ids = (item[1] != null) ? item[1] : [];
		let small_name = "unknown";
		for (let tag in category_tag_index) {
			if (("" + category_tag_index[tag]) == cat_id) {
				small_name = replace(tag, /^netify\./, "");
				break;
			}
		}
		small_category_to_apps[small_name] ??= [];
		for (let app_id in app_ids) {
			let id_str = "" + app_id;
			if (apps[id_str]) push(small_category_to_apps[small_name], apps[id_str]);
		}
	}

	for (let big_name in big_categories) {
		let all_apps = [], app_set = {};
		for (let small_cat in big_categories[big_name]) {
			if (small_category_to_apps[small_cat]) {
				for (let app_obj in small_category_to_apps[small_cat]) {
					if (!app_set[app_obj.name]) {
						push(all_apps, app_obj);
						app_set[app_obj.name] = true;
					}
				}
			}
		}
		sort(all_apps, function(a, b) {
			return (a.name < b.name) ? -1 : ((a.name > b.name) ? 1 : 0);
		});
		if (big_name == "sexual_content" || big_name == "gambling" || big_name == "malicious_content")
			all_apps = [];
		if (length(all_apps) > 0)
			push(result, { category_name: big_name, apps: all_apps });
	}
	sort(result, function(a, b) {
		return (a.category_name < b.category_name) ? -1 : ((a.category_name > b.category_name) ? 1 : 0);
	});
	return result;
}

// ---------------------------------------------------------------------------
// methods
// ---------------------------------------------------------------------------

export default {
	get_app_list: function(args, ctx) {
		return build_app_list(ctx);
	},

	add_group: function(args, ctx) {
		if (args.name == null || args.macs == null || args.schedules_enabled == null || args.enabled == null)
			return { err_code: -1, err_msg: "parameter missing" };
		if (args.schedules_enabled && args.schedules_days == null)
			return { err_code: -1, err_msg: "parameter missing" };

		let c = ctx.uci.cursor();
		let group_count = 0;
		c.foreach(CONFIG, "group", function(s) { group_count++; });
		if (group_count >= MAX_GROUPS)
			return { err_code: -1, err_msg: "the maximum number of groups has reached the limit of 10" };

		ctx.popen("mkdir -p " + RULE_DIR);

		let group_sid = c.add(CONFIG, "group");
		let nsid = random_uci_section(c, "group") ?? group_sid;
		c.rename(CONFIG, group_sid, nsid);
		group_sid = nsid;

		c.set(CONFIG, group_sid, "name", args.name);
		c.set(CONFIG, group_sid, "enabled", args.enabled ? "1" : "0");
		c.set(CONFIG, group_sid, "schedules_enabled", args.schedules_enabled ? "1" : "0");
		let sdays = (type(args.schedules_days) == "array" && length(args.schedules_days) != 0)
			? args.schedules_days : "";
		c.set(CONFIG, group_sid, "schedules_days", sdays);

		if (type(args.macs) == "array" && length(args.macs) != 0) {
			let macs = [];
			for (let m in args.macs) push(macs, uc(m));
			c.set(CONFIG, group_sid, "macs", macs);
		}

		if (type(args.schedules) == "array" && length(args.schedules) != 0) {
			for (let sch in args.schedules) {
				if (sch.week == null || sch.begin == null || sch["end"] == null || sch.enabled == null)
					return { err_code: -2, err_msg: "schedule parameter missing" };
				let sche = c.add(CONFIG, "schedule");
				let nsche = random_uci_section(c, "sche") ?? sche;
				c.rename(CONFIG, sche, nsche);
				sche = nsche;
				c.set(CONFIG, sche, "enabled", sch.enabled ? "1" : "0");
				c.set(CONFIG, sche, "group", group_sid);
				c.set(CONFIG, sche, "week", sch.week);
				c.set(CONFIG, sche, "begin", sch.begin);
				c.set(CONFIG, sche, "end", sch["end"]);
				c.set(CONFIG, sche, "rule", "drop");
			}
		}

		if (type(args.apps) == "array" || type(args.blacklist) == "array" || type(args.category) == "array") {
			let sid = c.add(CONFIG, "rule");
			let nrid = random_uci_section(c, "rule") ?? sid;
			c.rename(CONFIG, sid, nrid);
			sid = nrid;
			c.set(CONFIG, sid, "manual", (args.manual == false) ? "0" : "1");
			c.set(CONFIG, sid, "action", "POLICY_DROP");

			// DEGRADE: apps stored but only enforced if the netify DB is present.
			if (type(args.apps) == "array" && length(args.apps) != 0)
				fs.writefile(RULE_DIR + "/" + sid + "apps.conf", join("\n", args.apps) + "\n");

			// DEGRADE: category stored; enforcement needs the netify category DB.
			if (type(args.category) == "array" && length(args.category) != 0)
				c.set(CONFIG, sid, "category", args.category);

			if (type(args.blacklist) == "array" && length(args.blacklist) != 0) {
				let ll = get_long_list(args.blacklist);
				if (ll.long)
					return { err_code: -2, err_msg: "domain invalid", valid_list: ll.arr };
				let hu = handle_url(args.blacklist);
				if (hu.has)
					return { err_code: -2, err_msg: "domain invalid", valid_list: hu.arr };
				fs.writefile(RULE_DIR + "/" + sid + "blacklist.conf", join("\n", args.blacklist) + "\n");
			}
			c.set(CONFIG, group_sid, "default_rule", sid);
		}

		c.set(CONFIG, "global", "init", "0");
		c.set(CONFIG, "global", "enable", "1");
		c.commit(CONFIG);
		ctx.popen("sync");
		apply(ctx);
		return { id: group_sid };
	},

	remove_group: function(args, ctx) {
		if (args.id == null)
			return { err_code: -1, err_msg: "parameter missing" };

		let c = ctx.uci.cursor();
		let dead = [];
		c.foreach(CONFIG, "schedule", function(s) {
			if (s.group && s.group == args.id) push(dead, s[".name"]);
		});
		for (let s in dead) c.delete(CONFIG, s);

		let rule_name = c.get(CONFIG, args.id, "default_rule");
		if (rule_name) {
			ctx.popen("rm -f /tmp/" + rule_name + ".conf " +
				RULE_DIR + "/" + rule_name + "blacklist.conf " +
				RULE_DIR + "/" + rule_name + "apps.conf");
			c.delete(CONFIG, rule_name);
		}
		c.delete(CONFIG, args.id);
		c.commit(CONFIG);
		ctx.popen("sync");
		apply(ctx);
		return {};
	},

	set_group: function(args, ctx) {
		if (args.id == null)
			return { err_code: -1, err_msg: "parameter missing" };

		let c = ctx.uci.cursor();
		let sid = args.id;
		ctx.popen("mkdir -p " + RULE_DIR);

		if (args.enabled != null)
			c.set(CONFIG, sid, "enabled", args.enabled ? "1" : "0");
		if (args.schedules_enabled != null)
			c.set(CONFIG, sid, "schedules_enabled", args.schedules_enabled ? "1" : "0");
		if (args.schedules_days != null)
			c.set(CONFIG, sid, "schedules_days", args.schedules_days);
		if (args.name != null)
			c.set(CONFIG, sid, "name", args.name);

		if (args.macs != null) {
			if (type(args.macs) == "array" && length(args.macs) != 0) {
				let macs = [];
				for (let m in args.macs) push(macs, uc(m));
				c.set(CONFIG, sid, "macs", macs);
			} else {
				c.delete(CONFIG, sid, "macs");
			}
		}

		if (args.schedules != null) {
			let dead = [];
			c.foreach(CONFIG, "schedule", function(s) {
				if (s.group && s.group == args.id) push(dead, s[".name"]);
			});
			for (let s in dead) c.delete(CONFIG, s);

			if (type(args.schedules) == "array" && length(args.schedules) != 0) {
				for (let sch in args.schedules) {
					if (sch.week == null || sch.begin == null || sch["end"] == null || sch.enabled == null)
						return { err_code: -2, err_msg: "schedule parameter missing" };
					let sche = c.add(CONFIG, "schedule");
					let nsche = random_uci_section(c, "sche") ?? sche;
					c.rename(CONFIG, sche, nsche);
					sche = nsche;
					c.set(CONFIG, sche, "enabled", sch.enabled ? "1" : "0");
					c.set(CONFIG, sche, "group", sid);
					c.set(CONFIG, sche, "week", sch.week);
					c.set(CONFIG, sche, "begin", sch.begin);
					c.set(CONFIG, sche, "end", sch["end"]);
					c.set(CONFIG, sche, "rule", "drop");
				}
			}
		}

		let rule_name = c.get(CONFIG, args.id, "default_rule");
		if (args.blacklist != null && rule_name) {
			let path = RULE_DIR + "/" + rule_name + "blacklist.conf";
			if (type(args.blacklist) == "array" && length(args.blacklist) != 0) {
				let ll = get_long_list(args.blacklist);
				if (ll.long)
					return { err_code: -2, err_msg: "domain invalid", valid_list: ll.arr };
				let hu = handle_url(args.blacklist);
				if (hu.has)
					return { err_code: -2, err_msg: "domain invalid", valid_list: hu.arr };
				fs.writefile(path, join("\n", args.blacklist) + "\n");
			} else {
				ctx.popen("rm -f " + path);
			}
		}
		// DEGRADE: apps stored but only enforced when the netify DB is present.
		if (args.apps != null && rule_name) {
			let path = RULE_DIR + "/" + rule_name + "apps.conf";
			if (type(args.apps) == "array" && length(args.apps) != 0)
				fs.writefile(path, join("\n", args.apps) + "\n");
			else
				ctx.popen("rm -f " + path);
		}
		// DEGRADE: category stored; enforcement needs the netify category DB.
		if (args.category != null && rule_name) {
			if (type(args.category) == "array" && length(args.category) != 0)
				c.set(CONFIG, rule_name, "category", args.category);
			else
				c.delete(CONFIG, rule_name, "category");
		}

		c.commit(CONFIG);
		ctx.popen("sync");
		apply(ctx);
		return {};
	},

	get_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let enable = c.get(CONFIG, "global", "enable") ?? "0";
		let init = c.get(CONFIG, "global", "init") ?? "1";

		let groups = [];
		let by_id = {};
		c.foreach(CONFIG, "group", function(s) {
			let group = {
				id: s[".name"],
				name: s.name,
				enabled: s.enabled != "0",
				schedules_enabled: s.schedules_enabled != "0",
				blacklist: [],
				apps: [],
				category: [],
				schedules_days: []
			};
			if (s.macs) group.macs = as_list(s.macs);
			if (s.default_rule) {
				group.blacklist = read_lines(RULE_DIR + "/" + s.default_rule + "blacklist.conf");
				group.apps = read_lines(RULE_DIR + "/" + s.default_rule + "apps.conf");
				group.category = as_list(c.get(CONFIG, s.default_rule, "category"));
			}
			if (s.schedules_days != null) group.schedules_days = s.schedules_days;
			push(groups, group);
			by_id[group.id] = group;
		});

		c.foreach(CONFIG, "schedule", function(s) {
			let g = by_id[s.group];
			if (!g) return;
			g.schedules ??= [];
			push(g.schedules, {
				week: s.week,
				begin: s.begin,
				end: s["end"],
				enabled: s.enabled != "0"
			});
		});

		return {
			enable: enable != "0",
			groups: groups,
			init: init == "1"
		};
	},

	set_config: function(args, ctx) {
		if (args.enable == null)
			return { err_code: -1, err_msg: "parameter missing" };
		let c = ctx.uci.cursor();
		c.set(CONFIG, "global", "enable", args.enable ? "1" : "0");
		c.commit(CONFIG);
		ctx.popen("sync");
		apply(ctx);
		return {};
	}
};
