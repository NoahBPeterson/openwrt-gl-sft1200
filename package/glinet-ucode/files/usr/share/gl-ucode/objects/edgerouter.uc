// gl-ucode object: edgerouter  (port of decompiled-spec/rpc/edgerouter.lua)
// "Drop-in / bypass-gateway mode". get_config is plain uci on the `edgerouter`
// config (kept as our own store). DEGRADE: the GL `edgerouter` init script +
// /lib/functions/drop-in-function.sh check_dhcp_server (the drop-in DHCP probe /
// static-bypass enforcement) are absent on mainline, so set_config is store-only
// and get_status only reads /tmp/drop-in-dhcp-info if some external process wrote
// it. DROPPED: `atp mode_changed`, hardware.platform_get_wan_status cable check,
// and the vpn-client TAP-S2S cross-read (oui.network ipcalc reimplemented inline).
'use strict';

const fs = require('fs');

// --- IPv4 helpers (replace oui.network.ipcalc + gl.validator) ------------------

function ip_octets(ip) {
	if (type(ip) != "string") return null;
	let parts = split(ip, ".");
	if (length(parts) != 4) return null;
	let o = [];
	for (let p in parts) {
		if (!match(p, /^[0-9]+$/)) return null;
		let n = int(p);
		if (n < 0 || n > 255) return null;
		push(o, n);
	}
	return o;
}

function ip4addr(ip) { return ip_octets(ip) != null; }

function netmask4(m) {
	let o = ip_octets(m);
	if (!o) return false;
	let v = (o[0] << 24) | (o[1] << 16) | (o[2] << 8) | o[3];
	let inv = 4294967295 - v;                 // ~v within 32 bits
	return ((inv & (inv + 1)) == 0);          // mask == 1^n 0^m  <=>  inv == 2^k-1
}

// network address (octet array) for ip & mask, or null if either is malformed
function net_of(ip, mask) {
	let a = ip_octets(ip), m = ip_octets(mask);
	if (!a || !m) return null;
	return [ a[0] & m[0], a[1] & m[1], a[2] & m[2], a[3] & m[3] ];
}

// Faithful port of get_address_pool(): stores only the last octet as a string,
// matching the original (start = first usable host, end = last usable host).
function get_address_pool(gateway, mask) {
	let ret = {};
	let g = ip_octets(gateway), m = ip_octets(mask);
	if (!g || !m) return ret;
	let net = [ g[0] & m[0], g[1] & m[1], g[2] & m[2], g[3] & m[3] ];
	let bc  = [ net[0] | (255 - m[0]), net[1] | (255 - m[1]),
	            net[2] | (255 - m[2]), net[3] | (255 - m[3]) ];
	let start = net[3], ending = bc[3], gw = g[3];
	for (let i = start + 1; i <= ending - 1; i++) { if (i != gw) { ret.start = `${i}`; break; } }
	for (let i = ending - 1; i >= start + 1; i--) { if (i != gw) { ret.end = `${i}`; break; } }
	return ret;
}

// Port of check_config_valid(): is the probed subnet the same as our stored one?
function check_config_valid(ctx, ip, netmask) {
	let c = ctx.uci.cursor();
	let cip = c.get("edgerouter", "wandhcp", "ip") ?? "0.0.0.0";
	let cmask = c.get("edgerouter", "wandhcp", "netmask") ?? "0.0.0.0";
	let a = net_of(cip, cmask), b = net_of(ip, netmask);
	if (!a || !b) return false;
	return (a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3]);
}

// Port of check_params() validity portion. Returns true when params are MISSING
// (caller already gated on that) or valid; false only when present-but-invalid.
function check_params(p) {
	if (p.ip == null || p.netmask == null || p.gateway == null || p.dns == null)
		return true;
	if (!ip4addr(p.ip) || !netmask4(p.netmask) || !ip4addr(p.gateway)) return false;
	if (type(p.dns) != "array") return false;
	for (let d in p.dns) if (!ip4addr(d)) return false;
	return true;
}

export default {
	// obtain the status of LAN traffic passing through the edge (bypass) route.
	// DEGRADE: hardware.platform_get_wan_status() is gone, so detected:3 (cable
	// unplugged) is never emitted. DEGRADE: the drop-in daemon +
	// check_dhcp_server helper are absent, so we do NOT actively probe the
	// upstream DHCP server; we only parse /tmp/drop-in-dhcp-info if an external
	// process left it, otherwise detected:0 (disabled).
	get_status: function(args, ctx) {
		let ret = {};
		const path = "/tmp/drop-in-dhcp-info";
		let data = fs.access(path) ? fs.readfile(path) : null;
		if (!data) { ret.detected = 0; return ret; }

		ret.detected = 2;
		let c = ctx.uci.cursor();
		let our_ip = c.get("edgerouter", "wandhcp", "ip");
		let dns_gw = null, dns_alt = null;

		for (let line in split(data, "\n")) {
			let f = split(trim(line), /\s+/);
			if (!f || length(f) < 2 || f[0] == "") continue;
			if (f[0] == "router:") {
				if (our_ip != null && f[1] == our_ip) ret.detected = 1;
			} else if (f[0] == "yiaddr:") {
				ret.ip = f[1];
			} else if (f[0] == "giaddr:") {
				ret.gateway = f[1];
				dns_gw = f[1];                    // gateway is the preferred DNS
			} else if (f[0] == "netmask:") {
				ret.netmask = f[1];
			} else if (f[0] == "dns1:") {         // DHCP-issued DNS as fallback
				if (f[1] != "0.0.0.0" && f[1] != dns_gw) dns_alt = f[1];
			}
		}

		let dns = [];
		if (dns_gw != null) push(dns, dns_gw);
		if (dns_alt != null) push(dns, dns_alt);
		ret.dns = dns;
		ret.valid = check_config_valid(ctx, ret.ip, ret.netmask);
		return ret;
	},

	// modify the configuration. DEGRADE: store-only — the `edgerouter` init
	// script (drop-in DHCP probe + static bypass) is absent on mainline, so we
	// persist to the `edgerouter` uci config and best-effort kick the init
	// script only if it happens to exist. DROPPED: `atp mode_changed` and the
	// TAP-S2S cross-read of network.ovpnclient / ovpnclient.<cfg>.mode.
	set_config: function(args, ctx) {
		let params = args ?? {};
		if (type(params.enable) != "bool")
			return { err_code: 1, err_msg: "parameter missing" };

		if (params.enable) {
			if (params.ip == null || params.netmask == null ||
			    params.gateway == null || params.dns == null)
				return { err_code: 1, err_msg: "parameter missing" };
		}

		if (!check_params(params))
			return { err_code: 1, err_msg: "parameter missing" };

		let c = ctx.uci.cursor();
		c.set("edgerouter", "global", "global");
		c.set("edgerouter", "global", "enabled", params.enable ? "1" : "0");

		if (params.enable) {
			c.set("edgerouter", "wandhcp", "ignore", params.ignore ? "1" : "0");
			let pool = get_address_pool(params.gateway, params.netmask);
			c.set("edgerouter", "wandhcp", "dhcp");
			c.set("edgerouter", "wandhcp", "ip", params.ip);
			c.set("edgerouter", "wandhcp", "netmask", params.netmask);
			c.set("edgerouter", "wandhcp", "gateway", params.gateway);
			c.set("edgerouter", "wandhcp", "dns", params.dns);
			if (pool.start != null) c.set("edgerouter", "wandhcp", "start", pool.start);
			if (pool.end != null) c.set("edgerouter", "wandhcp", "end", pool.end);
		}

		c.commit("edgerouter");

		// DEGRADE: best-effort only; these init scripts are normally absent.
		if (fs.access("/etc/init.d/edgerouter"))
			ctx.popen("/etc/init.d/edgerouter restart >/dev/null 2>&1 &");
		if (fs.access("/etc/init.d/vpn-client"))
			ctx.popen("/etc/init.d/vpn-client restart >/dev/null 2>&1 &");

		return {};
	},

	// get the current stored configuration (plain uci reads).
	get_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let enable = c.get("edgerouter", "global", "enabled") ?? "0";
		let ignore = c.get("edgerouter", "wandhcp", "ignore");
		return {
			enable: (enable != "0"),
			ignore: (ignore != null && ignore != "0"),
			ip: c.get("edgerouter", "wandhcp", "ip"),
			netmask: c.get("edgerouter", "wandhcp", "netmask"),
			gateway: c.get("edgerouter", "wandhcp", "gateway"),
			dns: c.get("edgerouter", "wandhcp", "dns")
		};
	}
};
