// gl-ucode object: tailscale  (port of decompiled-spec/rpc/tailscale.lua)
// Tailscale mesh + exit nodes over the mainline `tailscale` pkg. Status/exit-node
// data is parsed from `tailscale status --json` via ctx.popen. GL-only bits degraded:
//   * glconfig.general.mode=="router" gate -> no-op/true (netmode not yet ported).
//   * gl-dns.@dns[0].mode DNS assembly -> fallback reading dhcp.@dnsmasq[0].server /
//     resolvfile (+ /tmp/resolv.conf.{ovpn,wg}).
//   * GL wrapper scripts /usr/bin/gl_tailscale + /usr/bin/ts_killswitch -> reimplemented
//     inline as `tailscale up` (--advertise-exit-node / --exit-node / --advertise-routes /
//     --snat-subnet-routes) plus an nft killswitch table.
//   * oui.network.ipcalc -> local ip math; oui.fs.sync -> ctx.popen("sync").
// If the `tailscale` binary is absent every getter degrades to sane defaults (no status).
'use strict';

const fs = require('fs');

// ---- ip math (oui.network.ipcalc replacement) ----------------------------
function ip2int(ip) {
	let p = split(ip, ".");
	if (length(p) != 4) return null;
	return int(p[0]) * 16777216 + int(p[1]) * 65536 + int(p[2]) * 256 + int(p[3]);
}

function int2ip(n) {
	return sprintf("%d.%d.%d.%d", (n >> 24) & 255, (n >> 16) & 255, (n >> 8) & 255, n & 255);
}

// dotted netmask ("255.255.255.0") or bare prefix ("24") -> prefix int
function to_prefix(m) {
	m = `${m}`;
	if (index(m, ".") >= 0) {
		let mi = ip2int(m), p = 0;
		while (mi) { p += mi & 1; mi = mi >> 1; }
		return p;
	}
	return int(m);
}

function cidr(ip, m) {
	let ipi = ip2int(ip);
	if (ipi == null) return null;
	let p = to_prefix(m);
	let maskbits = (p <= 0) ? 0 : ((0xffffffff << (32 - p)) & 0xffffffff);
	return int2ip(ipi & maskbits) + "/" + p;
}

function lan_cidr(ctx) {
	let c = ctx.uci.cursor();
	let ip = c.get("network", "lan", "ipaddr");
	let mask = c.get("network", "lan", "netmask");
	if (!ip || !mask) return null;
	return cidr(ip, mask);
}

// ---- validators (gl.validator replacement) -------------------------------
// validator.base(v, "boolean", true): optional boolean -> null OR real bool passes.
function is_opt_bool(v) { return v == null || type(v) == "bool"; }

function is_ip4(s) {
	let m = match(s, /^([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})$/);
	if (!m) return false;
	for (let i = 1; i <= 4; i++) if (int(m[i]) > 255) return false;
	return true;
}

// ---- `tailscale status --json` ------------------------------------------
// Returns the decoded object, or null when tailscale is absent/not answering.
function ts_status(ctx, wait) {
	wait = wait ?? 2;
	let data = ctx.popen(`timeout ${wait} tailscale status --json 2>/dev/null`);
	if (!data) return null;
	let t = trim(data);
	if (length(t) == 0 || substr(t, 0, 1) != "{") return null;   // guard json() throw
	let r = json(t);
	return (type(r) == "object") ? r : null;
}

// Build the exit-node view from a status snapshot (peers + node currently in use).
function exit_node_info(ctx) {
	let r = ts_status(ctx);
	let res = {};
	let list = [];

	if (!r || type(r.Peer) != "object") return res;

	for (let k in r.Peer) {
		let v = r.Peer[k];
		if (!v || !v.ExitNodeOption) continue;
		if (type(v.TailscaleIPs) != "array" || !v.TailscaleIPs[0]) continue;

		let node = { ip: v.TailscaleIPs[0] };

		if (v.Location && v.Location.Country) {
			let loc = v.Location.Country;
			if (v.Location.City) loc = loc + ", " + v.Location.City;
			node.location = loc;
		} else if (v.DNSName) {
			node.location = replace(v.DNSName, /\..*$/, "");   // strip from first dot
		} else {
			node.location = "Unknown location";
		}

		if (type(v.Tags) == "array") {
			for (let tag in v.Tags) {
				if (tag == "tag:mullvad-exit-node") { node.provider = "mullvad"; break; }
			}
		}

		push(list, node);
	}

	if (r.ExitNodeStatus && type(r.ExitNodeStatus.TailscaleIPs) == "array" && r.ExitNodeStatus.TailscaleIPs[0]) {
		let addr = r.ExitNodeStatus.TailscaleIPs[0];
		let pos = index(addr, "/");
		res.exit_node_ip = (pos >= 0) ? substr(addr, 0, pos) : addr;
	}

	res.exit_node_list = list;
	return res;
}

// parse "nameserver <ip>" lines from a resolv.conf-style file into `dns`.
function parse_resolv(path, dns) {
	if (!path || !fs.access(path)) return;
	let content = fs.readfile(path);
	if (!content) return;
	for (let line in split(content, "\n")) {
		let m = match(line, /nameserver (\S+)/);
		if (m) push(dns, m[1]);
	}
}

// ---- GL wrapper reimplementations ---------------------------------------
// DEGRADE: /usr/bin/ts_killswitch -> inline nft table that stops exit-node traffic
// leaking out the physical WAN. Engaged ONLY when an exit node is configured AND the
// tailscale0 interface exists, so it can never strand a box that isn't running tailscale.
function ts_killswitch(ctx) {
	if (!ctx.popen("command -v nft")) return;
	ctx.popen("nft delete table inet ts_killswitch 2>/dev/null");

	let c = ctx.uci.cursor();
	let enabled = c.get("tailscale", "settings", "enabled") == "1";
	let exit_ip = c.get("tailscale", "settings", "exit_node_ip") ?? "";
	if (!enabled || exit_ip == "") return;
	if (!fs.access("/sys/class/net/tailscale0")) return;

	ctx.popen("nft add table inet ts_killswitch 2>/dev/null");
	ctx.popen("nft add chain inet ts_killswitch fwd '{ type filter hook forward priority -150; policy accept; }' 2>/dev/null");
	ctx.popen("nft add rule inet ts_killswitch fwd oifname tailscale0 accept 2>/dev/null");
	ctx.popen("nft add rule inet ts_killswitch fwd iifname { br-lan, eth0.1 } oifname != tailscale0 ct state new drop 2>/dev/null");
}

// DEGRADE: /usr/bin/gl_tailscale restart -> (re)start the tailscale init script, then
// compose `tailscale up` from the uci settings. Runs `up` detached so a NeedsLogin
// re-up (get_auth_url) doesn't block while a fresh AuthURL is minted.
function ts_restart(ctx) {
	let c = ctx.uci.cursor();
	let enabled = c.get("tailscale", "settings", "enabled") == "1";

	if (fs.access("/etc/init.d/tailscale")) {
		if (enabled) {
			ctx.popen("/etc/init.d/tailscale enable >/dev/null 2>&1");
			ctx.popen("/etc/init.d/tailscale restart >/dev/null 2>&1");
		} else {
			ctx.popen("/etc/init.d/tailscale stop >/dev/null 2>&1");
			ctx.popen("/etc/init.d/tailscale disable >/dev/null 2>&1");
		}
	}
	if (!enabled) return;

	ctx.popen("sleep 2");

	let run_exit = c.get("tailscale", "settings", "run_exit_node") == "1";
	let exit_ip = c.get("tailscale", "settings", "exit_node_ip") ?? "";
	let lan_en = c.get("tailscale", "settings", "lan_enabled") == "1";
	let wan_en = c.get("tailscale", "settings", "wan_enabled") == "1";
	let masq = c.get("tailscale", "settings", "masq") == "1";

	let cmd = ["tailscale", "up", "--reset"];
	if (run_exit) push(cmd, "--advertise-exit-node");
	else if (exit_ip != "") push(cmd, `--exit-node=${exit_ip}`);
	if (lan_en) { let lc = lan_cidr(ctx); if (lc) push(cmd, `--advertise-routes=${lc}`); }
	if (wan_en) push(cmd, "--accept-routes");
	push(cmd, masq ? "--snat-subnet-routes=true" : "--snat-subnet-routes=false");

	ctx.popen(join(cmd, " ") + " >/dev/null 2>&1 &");
}

export default {
	// Get tailscale status. status: 1 login-required, 2 authorization-required,
	// 3 running, 4 connecting. Absent binary -> no status field (graceful).
	get_status: function(args, ctx) {
		let r = ts_status(ctx);
		let res = {};
		let dns = [];
		let c = ctx.uci.cursor();

		if (r && r.BackendState) {
			if (r.BackendState == "NeedsLogin") res.status = 1;
			if (r.BackendState == "NeedsMachineAuth") res.status = 2;
			if (r.BackendState == "Running") res.status = 3;
			if (r.BackendState == "Starting" || r.BackendState == "NoState" || r.BackendState == "InUseOtherUser") res.status = 4;

			if (type(r.User) == "object" && r.Self && type(r.Self) == "object" && r.Self.UserID) {
				for (let k in r.User) {
					let v = r.User[k];
					if (v && v.LoginName && v.ID && r.Self.UserID == v.ID)
						res.login_name = v.LoginName;
				}
			}

			if (type(r.TailscaleIPs) == "array" && r.TailscaleIPs[0])
				res.address_v4 = r.TailscaleIPs[0];
		}

		// DEGRADE: gl-dns.@dns[0].mode is a GL-only store (absent -> "auto"). Fall back
		// to dhcp.@dnsmasq[0].server (manual/proxy) or the resolvfile + vpn resolv (auto).
		let mode = c.get("gl-dns", "@dns[0]", "mode") ?? "auto";
		if (mode == "manual" || mode == "proxy") {
			let srv = c.get("dhcp", "@dnsmasq[0]", "server");
			if (type(srv) == "array") dns = srv;
			else if (srv) dns = [srv];
		} else if (mode == "auto") {
			parse_resolv(c.get("dhcp", "@dnsmasq[0]", "resolvfile") ?? "", dns);
			parse_resolv("/tmp/resolv.conf.ovpn", dns);
			parse_resolv("/tmp/resolv.conf.wg", dns);
		}

		// Fallback so dns is rarely empty on mainline (dnsmasq often has no resolvfile).
		if (length(dns) == 0) {
			let srv = c.get("dhcp", "@dnsmasq[0]", "server");
			if (type(srv) == "array") dns = srv;
			else if (srv) dns = [srv];
		}

		let tmp_dns = [];
		for (let item in dns) {
			if (!match(item, /:/)) {                    // skip IPv6 servers
				let mm = match(item, /([^#]+)/);        // strip #port / options
				push(tmp_dns, mm ? mm[1] : item);
			}
		}
		res.dns = tmp_dns;

		return res;
	},

	// Get tailscale login link. Restarts tailscale first so an expired url is refreshed.
	get_auth_url: function(args, ctx) {
		let r = ts_status(ctx);
		let res = {};

		if (r && r.BackendState == "NeedsLogin") {
			ts_restart(ctx);
			ctx.popen("sleep 3");
			let result = ts_status(ctx, 5);
			if (!result)
				return { err_code: 0, err_msg: "tailscale status error" };
			if (result.AuthURL) res.auth_url = result.AuthURL;
			return res;
		}

		return res;
	},

	get_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let res = {
			enabled: c.get("tailscale", "settings", "enabled") == "1",
			lan_enabled: c.get("tailscale", "settings", "lan_enabled") == "1",
			wan_enabled: c.get("tailscale", "settings", "wan_enabled") == "1",
			exit_node_ip: c.get("tailscale", "settings", "exit_node_ip"),
			masq: c.get("tailscale", "settings", "masq") == "1",
			run_exit_node: c.get("tailscale", "settings", "run_exit_node") == "1"
		};
		let lc = lan_cidr(ctx);
		if (lc) res.lan_ip = lc;
		return res;
	},

	get_exit_node_list: function(args, ctx) {
		let c = ctx.uci.cursor();
		let res = {};

		if (c.get("tailscale", "settings", "enabled") == "1") {
			let info = exit_node_info(ctx);
			if (info.exit_node_list) res.exit_node_list = info.exit_node_list;
			if (info.exit_node_ip) res.exit_node_ip = info.exit_node_ip;
		}

		return res;
	},

	set_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let enabled = args.enabled;
		let wan_enabled = args.wan_enabled;
		let lan_enabled = args.lan_enabled;
		let exit_node_ip = args.exit_node_ip;
		let run_exit_node = args.run_exit_node;
		let masq = args.masq;

		// GL OpenVPN TAP-S2S guard; degrades to a no-op when ovpnclient is absent.
		let config_name = c.get("network", "ovpnclient", "config");
		let client_mode = config_name ? (c.get("ovpnclient", config_name, "mode") ?? "") : "";
		if (client_mode == "tap-s2s") {
			let dis = c.get("network", "ovpnclient", "disabled") ?? "1";
			if (dis == "0")
				return { err_code: 20022002, err_msg: "function does not work because TAP-S2S Mode is enabled" };
		}

		if (!is_opt_bool(enabled)) return ctx.util.E.INVALID_PARAMS;        // invalid enabled
		if (!is_opt_bool(wan_enabled)) return ctx.util.E.INVALID_PARAMS;    // invalid wan_enabled
		if (!is_opt_bool(lan_enabled)) return ctx.util.E.INVALID_PARAMS;    // invalid lan_enabled
		if (exit_node_ip != "" && exit_node_ip != null && !is_ip4(exit_node_ip))
			return ctx.util.E.INVALID_PARAMS;                              // invalid exit_node_ip
		if (!is_opt_bool(run_exit_node)) return ctx.util.E.INVALID_PARAMS;  // invalid run_exit_node
		if (!is_opt_bool(masq)) return ctx.util.E.INVALID_PARAMS;          // invalid masq
		if (run_exit_node == true && exit_node_ip && exit_node_ip != "")
			return ctx.util.E.INVALID_PARAMS;                              // exit node conflict

		// DEGRADE: glconfig.general.mode=="router" gate dropped (netmode not yet ported);
		// tailscale is treated as always usable (router mode assumed).

		// ensure the settings section exists before writing options
		if (!c.get("tailscale", "settings")) c.set("tailscale", "settings", "settings");

		if (enabled) c.set("tailscale", "settings", "enabled", "1");
		c.set("tailscale", "settings", "lan_enabled", lan_enabled ? "1" : "0");
		c.set("tailscale", "settings", "wan_enabled", wan_enabled ? "1" : "0");
		c.set("tailscale", "settings", "masq", masq ? "1" : "0");

		if (run_exit_node) {
			c.set("tailscale", "settings", "run_exit_node", "1");
			c.set("tailscale", "settings", "exit_node_ip", "");
		} else {
			c.set("tailscale", "settings", "run_exit_node", "0");
			c.set("tailscale", "settings", "exit_node_ip", (exit_node_ip != null) ? exit_node_ip : "");
		}

		if (enabled == false) c.set("tailscale", "settings", "enabled", "0");

		c.commit("tailscale");
		ts_killswitch(ctx);
		ctx.popen("sync");
		ts_restart(ctx);
		return {};
	},

	logout: function(args, ctx) {
		ctx.popen("tailscale logout 2>/dev/null");

		let c = ctx.uci.cursor();
		c.set("tailscale", "settings", "exit_node_ip", "");
		c.commit("tailscale");
		ts_killswitch(ctx);

		let state_file = c.get("tailscale", "settings", "state_file");
		if (state_file && fs.access(state_file)) fs.unlink(state_file);
		ctx.popen("sync");
		return {};
	}
};
