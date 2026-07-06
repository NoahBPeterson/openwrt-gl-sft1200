// gl-ucode object: black_white_list  (port of decompiled-spec/rpc/black_white_list.lua)
// MAC allow/deny client blocking. Config store is kept identical to GL's
// `gl-black_white_list` uci (global.mode = black|white, black.mac[], white.mac[]),
// but the ENFORCEMENT is reimplemented over nftables instead of GL's
// /etc/init.d/gl-black_white_list.
// DEGRADE / dropped GL sources:
//   - /etc/init.d/gl-black_white_list (GL list->iptables translator, absent) is
//     replaced by apply() building an `inet gl_bwlist` nft table via ctx.popen.
//   - ubus.send("white_list_changed") (consumed by GL C daemons, absent) is
//     replaced by the apply() reload.
//   - utils.get_client_data_by_socket (GL gl_clients daemon socket, absent): the
//     requesting client's MAC is resolved from `ip neigh` / /proc/net/arp instead.
// PRESERVED: the whitelist admin-lockout safety check (refuse to enable a
// whitelist that does not contain the requesting client's own MAC).
'use strict';

const fs = require('fs');

const MAC_TYPE_ERR = -1;
const WHITE_INFO_ERR = -2;

const CONFIG = "gl-black_white_list";
const MAC_RE = /^[0-9a-fA-F]{2}(:[0-9a-fA-F]{2}){5}$/;

// uci list options come back as an array; a single value as a string; missing as
// null. Normalise everything to an array of macs.
function as_list(v) { // Returns: array | unknown // HOW? It literally cannot return anything BUT an array!
	if (type(v) == "array") return v;
	if (type(v) == "string" && length(v) > 0) return [v];
	return [];
}

// Make sure our store sections exist (this config does not ship on mainline).
function ensure(c) {
	if (!c.get(CONFIG, "global")) c.set(CONFIG, "global", "global");
	if (!c.get(CONFIG, "black")) c.set(CONFIG, "black", "black");
	if (!c.get(CONFIG, "white")) c.set(CONFIG, "white", "white");
}

// Resolve an IPv4 address to a MAC using the neighbour table (the GL client
// daemon is absent). Returns lowercase "aa:bb:cc:dd:ee:ff" or null.
function mac_of_ip(ctx, ip) {
	if (!ip) return null;
	let out = ctx.popen(`ip neigh show ${ip} 2>/dev/null`);
	if (type(out) == "string") {
		let m = match(out, /lladdr ([0-9a-fA-F:]{17})/);
		if (m) return lc(m[1]);
	}
	// fallback: /proc/net/arp  ("IP  HWtype  Flags  HWaddr  Mask  Device")
	let arp = fs.readfile("/proc/net/arp");
	if (type(arp) == "string") {
		for (let line in split(arp, "\n")) {
			let f = split(trim(line), /[ \t]+/);
			if (length(f) >= 4 && f[0] == ip && match(f[3], MAC_RE))
				return lc(f[3]);
		}
	}
	return null;
}

// Build (or tear down) the nftables enforcement from the current uci store.
// black mode -> drop LAN-origin traffic FROM listed macs.
// white mode -> drop LAN-origin traffic NOT from listed macs (allow-list).
function apply_rules(ctx) {
	let c = ctx.uci.cursor();
	let mode = c.get(CONFIG, "global", "mode");
	let macs = [];
	if (mode == "black") macs = as_list(c.get(CONFIG, "black", "mac"));
	else if (mode == "white") macs = as_list(c.get(CONFIG, "white", "mac"));

	// keep only well-formed macs so a junk entry cannot abort the whole ruleset
	let clean = [];
	for (let m in macs) if (type(m) == "string" && match(m, MAC_RE)) push(clean, lc(m));

	// always start from a clean slate (delete-if-exists without aborting)
	ctx.popen("nft add table inet gl_bwlist 2>/dev/null; nft delete table inet gl_bwlist 2>/dev/null");

	if ((mode != "black" && mode != "white") || length(clean) == 0)
		return true;   // nothing to enforce -> table left absent

	let op = (mode == "white") ? "!=" : "";   // whitelist matches the complement
	let ruleset =
		"table inet gl_bwlist {\n" +
		"\tset maclist {\n" +
		"\t\ttype ether_addr\n" +
		"\t\telements = { " + join(", ", clean) + " }\n" +
		"\t}\n" +
		"\tchain fwd {\n" +
		"\t\ttype filter hook forward priority filter; policy accept;\n" +
		`\t\tiifname "br-lan" ether saddr ${op} @maclist drop\n` +
		"\t}\n" +
		"\tchain inp {\n" +
		"\t\ttype filter hook input priority filter; policy accept;\n" +
		`\t\tiifname "br-lan" ether saddr ${op} @maclist drop\n` +
		"\t}\n" +
		"}\n";

	fs.writefile("/tmp/gl-bwlist.nft", ruleset);
	ctx.popen("nft -f /tmp/gl-bwlist.nft 2>/dev/null");
	return true;
}

export default {
	get_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		return {
			mode: c.get(CONFIG, "global", "mode"),
			black_mac: as_list(c.get(CONFIG, "black", "mac")),
			white_mac: as_list(c.get(CONFIG, "white", "mac"))
		};
	},

	set_single_mac: function(args, ctx) {
		let mode = args.mode;
		let op = args.operate;
		let mac = args.mac;
		if (mode != "black" && mode != "white")
			return ctx.util.E.INVALID_PARAMS;
		if (op != "add" && op != "del")
			return ctx.util.E.INVALID_PARAMS;
		if (type(mac) != "string" || !match(mac, MAC_RE))
			return ctx.util.E.INVALID_PARAMS;
		mac = lc(mac);

		let c = ctx.uci.cursor();
		ensure(c);
		let list = as_list(c.get(CONFIG, mode, "mac"));
		let next = [];
		let present = false;
		for (let m in list) {
			if (lc(m) == mac) { present = true; if (op == "del") continue; }
			push(next, m);
		}
		if (op == "add" && !present) push(next, mac);

		c.delete(CONFIG, mode, "mac");
		if (length(next) != 0) c.set(CONFIG, mode, "mac", next);
		c.commit(CONFIG);

		apply_rules(ctx);
		ctx.popen("sync");
		return {};
	},

	set_config: function(args, ctx) {
		let mode = args.mode;

		if (type(args.mac) != "array")
			return { err_code: MAC_TYPE_ERR, err_msg: "mac required" };
		if (mode && mode != "black" && mode != "white")
			return ctx.util.E.INVALID_PARAMS;

		let c = ctx.uci.cursor();
		ensure(c);
		c.set(CONFIG, "global", "mode", mode);

		if (mode == "black") {
			if (length(args.mac) != 0) c.set(CONFIG, "black", "mac", args.mac);
			else c.delete(CONFIG, "black", "mac");
		} else {
			// whitelist
			if (length(args.mac) == 0)
				return { err_code: -WHITE_INFO_ERR, err_msg: "white_list is null" };

			// admin-lockout safety: the requesting client's own MAC must be in the
			// list, else enabling a whitelist would lock the caller out of the router.
			let flag = false;
			let ra = (ctx.reqinfo && ctx.reqinfo.remote_addr) ? ctx.reqinfo.remote_addr : null;
			if (ra == "127.0.0.1" || (ctx.reqinfo && ctx.reqinfo.is_local)) {
				flag = true;
			} else {
				let remote_mac = mac_of_ip(ctx, ra);
				if (remote_mac) {
					for (let m in args.mac) {
						if (type(m) == "string" && lc(m) == remote_mac) { flag = true; break; }
					}
				}
			}
			if (!flag)
				return { err_code: -WHITE_INFO_ERR, err_msg: "white_list doesn't have a native mac" };

			c.set(CONFIG, "white", "mac", args.mac);
			// DEGRADE: ubus.send("white_list_changed") -> apply_rules() reload below.
		}

		c.commit(CONFIG);
		apply_rules(ctx);
		ctx.popen("sync");
		return {};
	},

	// Rebuild the nft enforcement from the current store (replaces the GL init.d
	// "start" the original spawned after every write). Safe to call at boot.
	apply: function(args, ctx) {
		apply_rules(ctx);
		return {};
	}
};
