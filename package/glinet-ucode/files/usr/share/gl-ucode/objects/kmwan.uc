// gl-ucode object: kmwan  (port of decompiled-spec/rpc/kmwan.lua)
// Multi-WAN failover/load-balance. The original drove GL's proprietary kmwan.ko
// (/proc/gl-kmwan/config) + /etc/init.d/kmwan daemon + `atp`/`gen_weight_route`
// helpers -- ALL absent on mainline (kmwan.ko is 4.14-locked). We keep a
// `kmwan`-shaped uci store as our own source of truth (so get/set_config and
// sensitivity round-trip), and where the OSS analog mwan3 is installed we
// translate that store into mwan3 uci (interfaces/members/policy/rule) and apply
// via /etc/init.d/mwan3 restart. If mwan3 is absent we degrade to store-only
// config + single-WAN/per-interface up-down status over ubus network.interface.
// DEGRADE: dropped kmwan.ko, /proc/gl-kmwan/config, atp interface_changed,
// gen_weight_route, sync_route_netcell, glmodem/glconfig(passthrough rtmode).
'use strict';

const fs = require('fs');

// map GL sensitivity level -> milliseconds (unchanged from original)
function level_ms(level, val) {
	if (level == "low") return 5000;
	if (level == "medium") return 3000;
	if (level == "high") return 1000;
	return int(val ?? 3) * 1000;   // custom: seconds -> ms
}

// v6 companion section name for an interface (modem uses "_6")
function iface6_of(iface) {
	return index(iface, "modem") >= 0 ? iface + "_6" : iface + "6";
}

// prepend "<cmd>," to every tracked ip, matching the original store format
function add_ip_info(cmd, ips) {
	let out = [];
	if (type(ips) == "array")
		for (let ip in ips) push(out, cmd + "," + ip);
	return out;
}

// strip the "<cmd>," prefix from a stored track entry -> bare ip
function track_ip(v) {
	let i = index(v, ",");
	return i >= 0 ? substr(v, i + 1) : v;
}

// ensure a kmwan section exists with the given type before setting options
function ensure(c, name, stype) {
	if (!c.get_all("kmwan", name)) c.set("kmwan", name, stype);
}

function has_mwan3() {
	return fs.access("/etc/init.d/mwan3") ? true : false;
}

// Rebuild mwan3 uci from our kmwan store and restart it. Best-effort; only
// called from set_* methods when mwan3 is present.
function sync_mwan3(c, ctx) {
	let mode = c.get("kmwan", "global", "mode") ?? "failover";
	let interval = int(int(c.get("kmwan", "global", "sensitivity") ?? "3000") / 1000);
	if (interval < 1) interval = 1;

	let members = [];
	c.foreach("kmwan", "member", function(s) {
		let name = s[".name"];
		if (index(name, "6") >= 0) return;   // skip v6 companion sections

		let ips = [];
		if (type(s.tracks) == "array")
			for (let v in s.tracks) push(ips, track_ip(v));

		let method = "ping";
		if (type(s.tracks) == "array" && s.tracks[0]) {
			let m = match(s.tracks[0], /^(\w+)/);
			if (m && m[1] != "ping") method = "httping";
		}

		// mwan3 interface tracker
		c.set("mwan3", name, "interface");
		c.set("mwan3", name, "enabled", (s.disabled == "0") ? "1" : "0");
		c.set("mwan3", name, "family", "ipv4");
		c.set("mwan3", name, "track_method", method);
		if (length(ips)) c.set("mwan3", name, "track_ip", ips);
		c.set("mwan3", name, "reliability", "1");
		c.set("mwan3", name, "count", "1");
		c.set("mwan3", name, "timeout", "" + interval);
		c.set("mwan3", name, "interval", "" + interval);
		c.set("mwan3", name, "up", "3");
		c.set("mwan3", name, "down", "3");

		// mwan3 member: metric drives failover order, weight drives balance
		let mname = name + "_m";
		c.set("mwan3", mname, "member");
		c.set("mwan3", mname, "interface", name);
		c.set("mwan3", mname, "metric", "" + (int(s.metric ?? "1")));
		c.set("mwan3", mname, "weight", "" + (int(s.weight ?? "1")));
		push(members, mname);
	});

	// single policy over all members (mwan3 balances equal-metric members by
	// weight and fails over across differing metrics)
	c.delete("mwan3", "gl_policy");
	c.set("mwan3", "gl_policy", "policy");
	if (length(members)) c.set("mwan3", "gl_policy", "use_member", members);
	c.set("mwan3", "gl_policy", "last_resort", (mode == "balance") ? "unreachable" : "default");

	// catch-all rule -> our policy
	c.delete("mwan3", "gl_rule");
	c.set("mwan3", "gl_rule", "rule");
	c.set("mwan3", "gl_rule", "dest_ip", "0.0.0.0/0");
	c.set("mwan3", "gl_rule", "use_policy", "gl_policy");

	c.commit("mwan3");
	ctx.popen("/etc/init.d/mwan3 restart >/dev/null 2>&1 &");
}

// mwan3 tracked-interface status -> { iface: "online"|... }. Empty if absent.
function mwan3_status(ctx) {
	let out = {};
	let s = ctx.popen("mwan3 status 2>/dev/null");
	if (type(s) != "string") return out;
	for (let line in split(s, "\n")) {
		let m = match(line, /interface (\S+) is (\w+)/);
		if (m) out[m[1]] = m[2];
	}
	return out;
}

// per-interface up-flag over netifd (the original's network.interface fallback)
function iface_up(ctx, iface) {
	let s = ctx.ubus.call(`network.interface.${iface}`, "status") ?? {};
	return s.up ? true : false;
}

export default {
	set_interface: function(params, ctx) {
		let interface = params.interface;
		if (!interface) return ctx.util.E.INVALID_PARAMS;
		let enable_check = params.enable_check;
		let track_mode = params.track_mode;
		let track_method = params.track_method;
		let track_proto = params.track_proto;
		let enable_ssl = params.enable_ssl;
		let track_ipv4 = params.track_ipv4;
		let track_ipv6 = params.track_ipv6;
		let iface6 = iface6_of(interface);

		let c = ctx.uci.cursor();
		ensure(c, interface, "member");
		ensure(c, iface6, "member");

		if (enable_check) {
			c.set("kmwan", interface, "check", "1");
			let tm = (track_mode == 0) ? "passive" : (track_mode == 1) ? "force" : "strict";
			c.set("kmwan", interface, "track_mode", tm);
			c.set("kmwan", iface6, "track_mode", tm);

			let track_cmd;
			if (track_method == 1) {
				track_cmd = "httping";
				c.set("kmwan", interface, "enable_ssl", enable_ssl ? "1" : "0");
				c.set("kmwan", iface6, "enable_ssl", enable_ssl ? "1" : "0");
			} else {
				track_cmd = "ping";
				c.delete("kmwan", interface, "enable_ssl");
				c.delete("kmwan", iface6, "enable_ssl");
			}

			if (track_proto == 0) {
				c.set("kmwan", interface, "disabled", "0");
				c.set("kmwan", iface6, "disabled", "1");
				c.set("kmwan", interface, "tracks", add_ip_info(track_cmd, track_ipv4));
			} else if (track_proto == 1) {
				c.set("kmwan", interface, "disabled", "1");
				c.set("kmwan", iface6, "disabled", "0");
				c.set("kmwan", iface6, "tracks", add_ip_info(track_cmd, track_ipv6));
			} else {
				c.set("kmwan", interface, "disabled", "0");
				c.set("kmwan", iface6, "disabled", "0");
				c.set("kmwan", interface, "tracks", add_ip_info(track_cmd, track_ipv4));
				c.set("kmwan", iface6, "tracks", add_ip_info(track_cmd, track_ipv6));
			}
		} else {
			c.set("kmwan", interface, "check", "0");
			c.set("kmwan", interface, "disabled", "1");
			c.set("kmwan", iface6, "disabled", "1");
		}
		c.commit("kmwan");

		if (has_mwan3()) sync_mwan3(c, ctx);
		return {};
	},

	set_sensitivity: function(params, ctx) {
		let s = params.sensitivity ?? {};
		let level = s.level ?? "medium";
		let val = level_ms(level, s.val);

		let c = ctx.uci.cursor();
		ensure(c, "global", "globals");
		c.set("kmwan", "global", "level", level);
		c.set("kmwan", "global", "sensitivity", "" + val);
		c.commit("kmwan");
		// DEGRADE: original also pushed {sensitivity,mode,rtmode} to
		// /proc/gl-kmwan/config -- kernel module absent, so store-only + mwan3.
		if (has_mwan3()) sync_mwan3(c, ctx);
		return {};
	},

	get_sensitivity: function(params, ctx) {
		let c = ctx.uci.cursor();
		let val = c.get("kmwan", "global", "sensitivity") ?? "3000";
		let level = c.get("kmwan", "global", "level") ?? "medium";
		return { sensitivity: { level: level, val: int(val) / 1000 } };
	},

	set_config: function(params, ctx) {
		let mode = params.mode;
		let args = params.interfaces ?? [];
		let c = ctx.uci.cursor();

		if (mode == 0) {
			for (let a in args) {
				let metric = a.metric;
				if (type(metric) != "int" && type(metric) != "double")
					return { err_code: -1, err_msg: "invalid metric" };
				if (metric < 0 || metric > length(args))
					return { err_code: -1, err_msg: "invalid metric" };
			}
			ensure(c, "global", "globals");
			c.set("kmwan", "global", "mode", "failover");
			for (let a in args) {
				let iface6 = iface6_of(a.interface);
				ensure(c, a.interface, "member");
				ensure(c, iface6, "member");
				c.set("kmwan", a.interface, "metric", "" + a.metric);
				c.set("network", a.interface, "metric", "" + a.metric);
				c.set("kmwan", iface6, "metric", "" + a.metric);
			}
			c.commit("network");
		} else {
			for (let a in args) {
				let weight = a.weight;
				if (type(weight) != "int" && type(weight) != "double")
					return { err_code: -1, err_msg: "invalid weight" };
				if (weight < 0 || weight > 10)
					return { err_code: -1, err_msg: "invalid weight" };
			}
			ensure(c, "global", "globals");
			c.set("kmwan", "global", "mode", "balance");
			for (let a in args) {
				let iface6 = iface6_of(a.interface);
				ensure(c, a.interface, "member");
				ensure(c, iface6, "member");
				c.set("kmwan", a.interface, "weight", "" + a.weight);
				c.set("kmwan", iface6, "weight", "" + a.weight);
			}
		}
		c.commit("kmwan");

		ctx.ubus.call("network", "reload");
		if (has_mwan3()) sync_mwan3(c, ctx);
		return {};
	},

	get_config: function(params, ctx) {
		let c = ctx.uci.cursor();
		let mode = (c.get("kmwan", "global", "mode") == "failover") ? 0 : 1;
		let ipv6_enable = c.get("glipv6", "globals", "enabled");
		let interfaces = [];

		c.foreach("kmwan", "member", function(s) {
			let name = s[".name"];
			if (index(name, "6") >= 0) return;   // skip v6 companion sections
			let iface6 = iface6_of(name);

			let track_ipv4 = [];
			if (type(s.tracks) == "array")
				for (let v in s.tracks) push(track_ipv4, track_ip(v));

			let tracks6 = c.get("kmwan", iface6, "tracks");
			let track_ipv6 = [];
			if (type(tracks6) == "array")
				for (let v in tracks6) push(track_ipv6, track_ip(v));

			let track_method = 1;
			if (type(s.tracks) == "array" && s.tracks[0]) {
				let m = match(s.tracks[0], /^(\w+)/);
				track_method = (m && m[1] == "ping") ? 0 : 1;
			}
			let enable_ssl = c.get("kmwan", name, "enable_ssl") == "1";

			let res = {
				interface: name,
				metric: int(s.metric ?? 0),
				weight: int(s.weight ?? 0),
				track_mode: (s.track_mode == "force") ? 1 : (s.track_mode == "passive") ? 0 : 2,
				track_method: track_method,
				enable_ssl: (track_method == 1) ? enable_ssl : false,
				track_ipv4: track_ipv4,
				track_ipv6: track_ipv6
			};

			let enable_check = s.disabled == "0";
			let enable_check6 = false;
			if (ipv6_enable == "1")
				enable_check6 = c.get("kmwan", iface6, "disabled") == "0";
			res.enable_check = enable_check || enable_check6;
			if (enable_check && !enable_check6) res.track_proto = 0;
			else if (!enable_check && enable_check6) res.track_proto = 1;
			else if (enable_check && enable_check6) res.track_proto = 2;

			push(interfaces, res);
		});
		return { mode: mode, interfaces: interfaces };
	},

	get_status: function(params, ctx) {
		let c = ctx.uci.cursor();
		let interfaces = [];
		let mstat = has_mwan3() ? mwan3_status(ctx) : {};
		let found = false;

		c.foreach("kmwan", "member", function(s) {
			let iface = s[".name"];
			if (index(iface, "6") >= 0) return;   // skip v6 companion sections
			found = true;
			let iface6 = iface6_of(iface);

			let up4, up6;
			if (exists(mstat, iface)) {
				up4 = (mstat[iface] == "online");
				up6 = exists(mstat, iface6) ? (mstat[iface6] == "online") : iface_up(ctx, iface6);
			} else {
				// DEGRADE: no mwan3 tracker -> per-interface netifd up/down
				up4 = iface_up(ctx, iface);
				up6 = iface_up(ctx, iface6);
			}
			push(interfaces, {
				interface: iface,
				status_v4: up4 ? 0 : 1,
				status_v6: up6 ? 0 : 1
			});
		});

		// DEGRADE: single-WAN device with no kmwan store -> report wan directly
		if (!found) {
			let up4 = exists(mstat, "wan") ? (mstat["wan"] == "online") : iface_up(ctx, "wan");
			let up6 = exists(mstat, "wan6") ? (mstat["wan6"] == "online") : iface_up(ctx, "wan6");
			if (up4 || up6 || ctx.ubus.call("network.interface.wan", "status"))
				push(interfaces, { interface: "wan", status_v4: up4 ? 0 : 1, status_v6: up6 ? 0 : 1 });
		}
		return { interfaces: interfaces };
	}
};
