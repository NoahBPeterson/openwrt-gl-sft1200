// gl-ucode object: zerotier  (port of decompiled-spec/rpc/zerotier.lua)
// Joins a ZeroTier network via the stock `zerotier` pkg + stock network/firewall
// uci. GL-only bits degraded: oui.network.ipcalc -> local ip math; the <21 ifname
// binding branch is DROPPED (25.12 always binds via network.zerotier.device);
// flush_zerotier_nf_conntrack C helper -> `conntrack -D` popen; ngx sleeps/spawns
// -> ctx.popen. The ovpnclient TAP-S2S guard degrades to a no-op when absent.
'use strict';

const fs = require('fs');

function ip2int(ip) {
	let p = split(ip, ".");
	if (length(p) != 4) return null;
	return int(p[0]) * 16777216 + int(p[1]) * 65536 + int(p[2]) * 256 + int(p[3]);
}

function int2ip(n) {
	return sprintf("%d.%d.%d.%d", (n >> 24) & 255, (n >> 16) & 255, (n >> 8) & 255, n & 255);
}

// accepts a dotted netmask ("255.255.255.0") or a bare prefix ("24") -> prefix int
function to_prefix(m) {
	m = `${m}`;
	if (index(m, ".") >= 0) {
		let mi = ip2int(m), p = 0;
		while (mi) { p += mi & 1; mi = mi >> 1; }
		return p;
	}
	return int(m);
}

// oui.network.ipcalc replacement: "network/prefix" from an ip + mask/prefix
function cidr(ip, m) {
	let ipi = ip2int(ip);
	if (ipi == null) return null;
	let p = to_prefix(m);
	let maskbits = (p <= 0) ? 0 : ((0xffffffff << (32 - p)) & 0xffffffff);
	return int2ip(ipi & maskbits) + "/" + p;
}

function wan_cidr(ctx, iface) {
	let s = ctx.ubus.call(`network.interface.${iface}`, "status");
	if (!s || !s["ipv4-address"] || !s["ipv4-address"][0]) return null;
	let a = s["ipv4-address"][0];
	if (!a.address) return null;
	return cidr(a.address, a.mask);
}

// scan /sys/class/net for the zt* device (up to 5 tries), bind it and build the
// stock `zerotier` firewall zone. 25.12: always bind via network.zerotier.device.
function zerotier_init(ctx) {
	let c = ctx.uci.cursor();
	for (let attempt = 0; attempt < 5; attempt++) {
		for (let ifname in fs.lsdir("/sys/class/net")) {
			if (match(ifname, /^zt/)) {
				c.set("network", "zerotier", "interface");
				c.set("network", "zerotier", "device", ifname);
				c.set("network", "zerotier", "proto", "none");
				c.set("firewall", "zerotier", "zone");
				c.set("firewall", "zerotier", "name", "zerotier");
				c.set("firewall", "zerotier", "network", ["zerotier"]);
				c.set("firewall", "zerotier", "input", "ACCEPT");
				c.set("firewall", "zerotier", "forward", "REJECT");
				c.set("firewall", "zerotier", "masq", "1");
				c.set("firewall", "zerotier", "mtu_fix", "1");
				c.set("firewall", "zerotier", "output", "ACCEPT");
				c.set("firewall", "zerotier", "enabled", "1");
				c.commit("network");
				c.commit("firewall");
				ctx.popen("sync");
				return;
			}
		}
		ctx.popen("sleep 1");
	}
}

function zerotier_lan_nat_start(ctx) {
	let c = ctx.uci.cursor();
	c.set("firewall", "zerotier_lan", "forwarding");
	c.set("firewall", "zerotier_lan", "dest", "lan");
	c.set("firewall", "zerotier_lan", "src", "zerotier");
	c.set("firewall", "lan_zerotier", "forwarding");
	c.set("firewall", "lan_zerotier", "dest", "zerotier");
	c.set("firewall", "lan_zerotier", "src", "lan");
	c.commit("firewall");
}

function zerotier_lan_nat_stop(ctx) {
	let c = ctx.uci.cursor();
	c.delete("firewall", "zerotier_lan");
	c.delete("firewall", "lan_zerotier");
	c.commit("firewall");
}

function zerotier_wan_nat_start(ctx) {
	let c = ctx.uci.cursor();
	c.set("firewall", "zerotier_wan", "forwarding");
	c.set("firewall", "zerotier_wan", "dest", "wan");
	c.set("firewall", "zerotier_wan", "src", "zerotier");
	c.commit("firewall");
}

function zerotier_wan_nat_stop(ctx) {
	let c = ctx.uci.cursor();
	c.delete("firewall", "zerotier_wan");
	c.commit("firewall");
}

export default {
	get_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let join = c.get("zerotier", "gl", "join");
		let id = null;
		if (type(join) == "array") { for (let v in join) id = v; }   // uci list -> last element
		else if (join) id = join;
		return {
			enabled: (c.get("zerotier", "gl", "enabled") == "1"),
			lan_enabled: (c.get("zerotier", "gl", "nat") == "1"),
			wan_enabled: (c.get("zerotier", "gl", "wan_nat") == "1"),
			id: id
		};
	},

	set_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let enabled = args.enabled;
		let lan_enabled = args.lan_enabled;
		let wan_enabled = args.wan_enabled;
		let id = args.id;

		// GL OpenVPN TAP-S2S guard. Degrades to a no-op when ovpnclient is absent.
		let config_name = c.get("network", "ovpnclient", "config");
		let client_mode = config_name ? (c.get("ovpnclient", config_name, "mode") ?? "") : "";
		if (client_mode == "tap-s2s") {
			let tap_s2s_disabled = c.get("network", "ovpnclient", "disabled") ?? "1";
			if (tap_s2s_disabled == "0")
				return {
					err_code: 20017003,
					err_msg: "function does not work because TAP-S2S Mode is enabled"
				};
		}

		if (type(enabled) != "bool")
			return ctx.util.E.INVALID_PARAMS;   // "invalid enabled"

		if (enabled) {
			if (type(lan_enabled) != "bool")
				return ctx.util.E.INVALID_PARAMS;   // "invalid lan_enabled"
			if (type(wan_enabled) != "bool")
				return ctx.util.E.INVALID_PARAMS;   // "invalid wan_enabled"
			if (type(id) != "string" || length(id) != 16 || !match(id, /^[0-9a-fA-F]+$/))
				return ctx.util.E.INVALID_PARAMS;   // "invalid id"
		}

		if (enabled) {
			c.set("zerotier", "gl", "enabled", "1");
			c.set("zerotier", "gl", "nat", lan_enabled ? "1" : "0");
			c.set("zerotier", "gl", "wan_nat", wan_enabled ? "1" : "0");
			c.set("firewall", "zerotier", "enabled", "1");

			let id_change = false;
			let join = c.get("zerotier", "gl", "join");
			if (type(join) == "array") { for (let v in join) if (v != id) id_change = true; }
			else if (join && join != id) id_change = true;

			c.set("zerotier", "gl", "join", [id]);
			c.commit("zerotier");
			ctx.popen("sleep 1");
			ctx.popen("/etc/init.d/zerotier reload");

			let proto = c.get("network", "zerotier", "proto");
			if (proto == null || id_change) {
				ctx.popen("sleep 6");
				zerotier_init(ctx);
			}

			if (lan_enabled) zerotier_lan_nat_start(ctx);
			else zerotier_lan_nat_stop(ctx);

			if (wan_enabled) zerotier_wan_nat_start(ctx);
			else zerotier_wan_nat_stop(ctx);

			ctx.ubus.call("network", "reload");
			// DEGRADE: flush_zerotier_nf_conntrack C helper -> conntrack -D, run
			// 5s later in the background (mirrors the original ngx.timer.at(5,...)).
			ctx.popen("(sleep 5; conntrack -D >/dev/null 2>&1) &");
		} else {
			c.set("zerotier", "gl", "enabled", "0");
			c.set("firewall", "zerotier", "enabled", "0");
		}

		c.commit("zerotier");
		c.commit("firewall");
		ctx.popen("/etc/init.d/firewall reload");
		ctx.popen("/etc/init.d/zerotier reload");
		ctx.popen("sync");
		return {};
	},

	get_status: function(args, ctx) {
		let c = ctx.uci.cursor();
		let status = null;
		let zerotier_ip = null, lan_ip = null, wan_ip = null;
		let wwan_ip = null, secondwan_ip = null, usbwan_ip = null;

		let zerotier_info = ctx.popen("zerotier-cli listnetworks | tail -n 1");
		if (zerotier_info) {
			let m = match(zerotier_info, /([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)/);
			if (m) zerotier_ip = m[1];

			if (match(zerotier_info, /OK/)) {
				lan_ip = cidr(c.get("network", "lan", "ipaddr") ?? "", c.get("network", "lan", "netmask") ?? "");
				wan_ip = wan_cidr(ctx, "wan");
				secondwan_ip = wan_cidr(ctx, "secondwan");
				wwan_ip = wan_cidr(ctx, "wwan");
				usbwan_ip = wan_cidr(ctx, "usbwan");
				status = 0;
			} else if (match(zerotier_info, /REQUESTING_CONFIGURATION/)) {
				status = 1;
			} else if (match(zerotier_info, /ACCESS_DENIED/)) {
				let secret = c.get("zerotier", "gl", "secret");
				if (secret && zerotier_ip) {
					c.delete("zerotier", "gl", "secret");
					c.commit("zerotier");
					ctx.popen("/etc/init.d/zerotier reload");
				}
				status = 2;
			} else {
				status = 3;
			}
		}

		return {
			status: status,
			zerotier_ip: zerotier_ip,
			lan_ip: lan_ip,
			wan_ip: wan_ip,
			secondwan_ip: secondwan_ip,
			wwan_ip: wwan_ip,
			usbwan_ip: usbwan_ip
		};
	}
};
