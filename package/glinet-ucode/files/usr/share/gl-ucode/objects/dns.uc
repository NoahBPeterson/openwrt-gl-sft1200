// gl-ucode object: dns  (port of decompiled-spec/rpc/dns.lua)
// DNS settings (auto/manual/proxy/secure) + /etc/hosts editor. The GL state block
// is kept in a `gl-dns-v2`-shaped uci config as our OWN store (mode/provider/proto/
// lists), and TRANSLATED to stock `dhcp/@dnsmasq[0]` (server/noresolv/cachesize/
// localuse/rebind_protection). force_dns is enforced as a fw4 :53 DNAT redirect.
// DEGRADE / dropped GL sources:
//   - SECURE mode (DoH/DoT/DoQ/DNSCrypt/ODoH): get_info STILL returns the full
//     provider catalog, but set_config(mode=secure) is REJECTED with a clear error
//     when the backing packages (dnsproxy / dnscrypt-proxy) are absent. We never
//     write their config files (dnsproxy.yaml / dnscrypt-proxy.toml); when a backend
//     IS installed we only store GL state + point dnsmasq at 127.0.0.1#5453 and let
//     the package own its own upstream config.
//   - The modem server-list branch (cellular.modem / /proc/gl-hw-info/modem-cpu) is
//     DROPPED (no modem on this device).
//   - vpn_client.set_dns_override_vpn (GL C helper) is dropped; override_vpn is still
//     stored so the app round-trips it.
//   - get_info provider server-lists live in /etc/dnsproxy/provider/*: read if present,
//     else degrade to a {provider, sup_proto} stub so the catalog shape survives.
'use strict';

const fs = require('fs');

const CFG = "gl-dns-v2";
const SEC = "@dns[0]";
const PROVIDER_DIR = "/etc/dnsproxy/provider/";

const ERRORS = {
	DNS_BUSY: -1,
	FETCH_SERVER_LIST_FAILED: -2,
	SERVER_NOT_FOUND: -3,
	SERVER_CHECK_FAIL: -4,
	PARAMETER_INVALID: -5,
	SECURE_UNAVAILABLE: -6
};

const PROVIDERS = [ "controld", "nextdns", "quad9", "cleanbrowsing",
	"cloudflare", "adguard_dns", "google_dns", "opendns", "manual" ];

// Fallback supported-protocol lists used when the provider server-list file is
// absent (so get_info still returns a usable catalog shape for the app).
const SUP = {
	controld:      [ "dot", "doh", "doq" ],
	nextdns:       [ "dot", "doh", "doq" ],
	quad9:         [ "dot", "doh" ],
	cleanbrowsing: [ "dot", "doh" ],
	cloudflare:    [ "dot", "doh", "doq" ],
	adguard_dns:   [ "dot", "doh", "doq", "dnscrypt" ],
	google_dns:    [ "dot", "doh" ],
	opendns:       [ "dot", "doh" ],
	manual:        [ "dot", "doh", "doq", "odoh", "dnscrypt" ]
};

// uci list options: array when multi, string when single, null when missing.
function as_list(v) {
	if (type(v) == "array") return v;
	if (type(v) == "string" && length(v) > 0) return [v];
	return [];
}

// Optional boolean: absent OR a real bool.
function bool_opt(v) {
	return v == null || type(v) == "bool";
}

// gl-dns-v2 does not ship on mainline; make sure our anonymous @dns[0] exists.
function ensure_store(c) {
	if (c.get(CFG, SEC) == null)
		c.add(CFG, "dns");
}

function is_ipv4(s) {
	if (type(s) != "string") return false;
	let m = match(s, /^([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})$/);
	if (!m) return false;
	for (let i = 1; i <= 4; i++) if (int(m[i]) > 255) return false;
	return true;
}

function is_ipv6(s) {
	// loose check: hex groups + at least one colon (dash-free char class).
	return type(s) == "string" && index(s, ":") >= 0 && match(s, /^[0-9a-fA-F:.]+$/);
}

function is_ipaddr(s) {
	return is_ipv4(s) || is_ipv6(s);
}

// Parse the dnsmasq resolvfile + VPN resolv files into res.server_auto, a list of
// "iface ns1,ns2" strings. Modem branch dropped; lan/lan_dhcp skipped; IPv6 names
// gated on glipv6.globals.enabled (as the original).
function get_auto_config(ctx, c, res) {
	res.server_auto = [];
	let ipv6_en = c.get("glipv6", "globals", "enabled");

	let resolvfile = c.get("dhcp", "@dnsmasq[0]", "resolvfile")
		?? "/tmp/resolv.conf.d/resolv.conf.auto";

	let server = {};    // iface -> [nameservers]
	if (fs.access(resolvfile)) {
		let data = fs.readfile(resolvfile) ?? "";
		let cur = null;
		for (let l in split(data, "\n")) {
			let mi = match(l, /# Interface (\S+)/);
			if (mi) {
				server[mi[1]] = [];
				cur = mi[1];
			} else {
				let mn = match(l, /nameserver (\S+)/);
				if (mn && cur != null) push(server[cur], mn[1]);
				else cur = null;
			}
		}
		for (let name in server) {
			let ns = server[name];
			if (length(ns) == 0) continue;
			// DEGRADE: modem rename branch dropped (no cellular).
			let is6 = match(name, /6$/) ? true : false;
			if (name == "lan" || name == "lan_dhcp") continue;
			if (is6 && ipv6_en == "0") continue;
			push(res.server_auto, name + " " + join(",", ns));
		}
	}

	// VPN client resolv (ovpnclient* / wgclient*): files usually absent -> no-op.
	c.foreach("network", "interface", function(s) {
		let nm = s[".name"];
		if (!nm) return;
		if (!(index(nm, "ovpnclient") >= 0 || index(nm, "wgclient") >= 0)) return;
		if (c.get("network", nm, "disabled") != "0") return;
		let path = "/tmp/resolv.conf.d/resolv.conf." + nm;
		let v4 = [], v6 = [];
		if (fs.access(path)) {
			for (let l in split(fs.readfile(path) ?? "", "\n")) {
				let mn = match(l, /nameserver (\S+)/);
				if (mn) {
					if (index(mn[1], ":") >= 0) push(v6, mn[1]);
					else push(v4, mn[1]);
				}
			}
		}
		if (length(v4) > 0) push(res.server_auto, nm + " " + join(",", v4));
		if (length(v6) > 0 && ipv6_en == "1") push(res.server_auto, nm + "6 " + join(",", v6));
	});

	return res;
}

// Build (or tear down) the force-DNS interception: LAN udp/tcp dport 53 -> router.
function apply_force_dns(ctx, enable) {
	let c = ctx.uci.cursor();
	c.delete("firewall", "gl_force_dns_udp");
	c.delete("firewall", "gl_force_dns_tcp");
	if (enable) {
		for (let proto in [ "udp", "tcp" ]) {
			let id = "gl_force_dns_" + proto;
			c.set("firewall", id, "redirect");
			c.set("firewall", id, "name", "GL-Force-DNS-" + proto);
			c.set("firewall", id, "src", "lan");
			c.set("firewall", id, "proto", proto);
			c.set("firewall", id, "src_dport", "53");
			c.set("firewall", id, "dest_port", "53");
			c.set("firewall", id, "target", "DNAT");
			c.set("firewall", id, "family", "ipv4");
		}
	}
	c.commit("firewall");
	ctx.popen("/etc/init.d/firewall reload >/dev/null 2>&1");
}

export default {
	// Provider catalog. Shape preserved for the app (array of {provider, sup_proto, ...}).
	get_info: function(args, ctx) {
		let c = ctx.uci.cursor();
		let list = [];
		for (let p in PROVIDERS) {
			if (p == "manual") {
				push(list, {
					provider: "manual",
					sup_proto: SUP.manual,
					proto_manual: c.get(CFG, SEC, "proto_manual") ?? "",
					secure_manual_list: as_list(c.get(CFG, SEC, "secure_manual_list"))
				});
			} else if (p == "nextdns") {
				push(list, { provider: "nextdns", sup_proto: SUP.nextdns });
			} else {
				let content = fs.readfile(PROVIDER_DIR + p);
				let obj = null;
				if (content) {
					try { obj = json(content); }
					catch (e) { obj = null; }
				}
				// DEGRADE: provider server-list file absent -> name + sup_proto stub.
				push(list, obj ?? { provider: p, sup_proto: SUP[p] ?? [] });
			}
		}
		return list;
	},

	get_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let res = {};
		res.force_dns = c.get(CFG, SEC, "force_dns") == "1";
		res.rebind_protection = c.get("dhcp", "@dnsmasq[0]", "rebind_protection") != "0";
		res.override_vpn = c.get(CFG, SEC, "override_vpn") == "1";
		let mode = c.get(CFG, SEC, "mode") ?? "auto";
		res.mode = mode;
		res.proto = c.get(CFG, SEC, "proto") ?? "";
		res.proto_manual = c.get(CFG, SEC, "proto_manual") ?? "";
		res.provider = c.get(CFG, SEC, "provider") ?? "";
		res.proxy_list = as_list(c.get(CFG, SEC, "proxy_list"));
		res.manual_list = as_list(c.get(CFG, SEC, "manual_list"));
		res.servers_list = as_list(c.get(CFG, SEC, "servers_list"));
		res.secure_manual_list = as_list(c.get(CFG, SEC, "secure_manual_list"));
		res.nextdns_id = c.get(CFG, SEC, "nextdns_id") ?? "";
		res.controld_id = c.get(CFG, SEC, "controld_id") ?? "";
		res.controld_type = int(c.get(CFG, SEC, "controld_type") ?? "1");
		if (mode == "auto") get_auto_config(ctx, c, res);
		return res;
	},

	set_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		ensure_store(c);
		let mode = args.mode;
		let proto = args.proto;
		let provider = args.provider;

		if (!bool_opt(args.force_dns))         return ctx.util.E.INVALID_PARAMS;
		if (!bool_opt(args.override_vpn))      return ctx.util.E.INVALID_PARAMS;
		if (!bool_opt(args.rebind_protection)) return ctx.util.E.INVALID_PARAMS;

		let modes = { auto: true, secure: true, manual: true, proxy: true };
		if (mode != null && !modes[mode]) return ctx.util.E.INVALID_PARAMS;

		if (mode != null) {
			// Refuse to fight AdGuard Home over the dnsmasq upstream.
			if (c.get("adguardhome", "config", "enabled") == "1")
				return { err_code: ERRORS.DNS_BUSY, err_msg: "Dns Busy. Occupied by adguardhome" };
			c.delete("dhcp", "@dnsmasq[0]", "server");
			c.delete("dhcp", "@dnsmasq[0]", "noresolv");
			c.delete("dhcp", "@dnsmasq[0]", "boguspriv");
			c.delete("dhcp", "@dnsmasq[0]", "cachesize");
			c.delete("dhcp", "@dnsmasq[0]", "localuse");
			ctx.popen("ln -sf \"/tmp/resolv.conf.d/resolv.conf.auto\" /tmp/resolv.conf");
		}

		if (mode == "manual") {
			let ml = as_list(args.manual_list);
			if (length(ml) == 0)
				return ctx.util.E.INVALID_PARAMS;
			for (let ip in ml)
				if (!is_ipaddr(ip)) return ctx.util.E.INVALID_PARAMS;
			c.set("dhcp", "@dnsmasq[0]", "noresolv", "1");
			c.set("dhcp", "@dnsmasq[0]", "server", ml);
			c.set(CFG, SEC, "manual_list", ml);

		} else if (mode == "proxy") {
			let pl = as_list(args.proxy_list);
			if (length(pl) == 0)
				return ctx.util.E.INVALID_PARAMS;
			for (let proxy in pl) {
				let parts = split(proxy, "#");
				if (length(parts) != 2 || parts[0] == "" || parts[1] == "")
					return ctx.util.E.INVALID_PARAMS;
				if (!is_ipaddr(parts[0]))
					return ctx.util.E.INVALID_PARAMS;
				let port = int(parts[1]);
				if (port < 0 || port > 65535)
					return ctx.util.E.INVALID_PARAMS;
			}
			c.set("dhcp", "@dnsmasq[0]", "noresolv", "1");
			c.set("dhcp", "@dnsmasq[0]", "server", pl);
			c.set(CFG, SEC, "proxy_list", pl);

		} else if (mode == "secure") {
			let providers = {
				controld: true, nextdns: true, quad9: true, cleanbrowsing: true,
				cloudflare: true, adguard_dns: true, google_dns: true,
				opendns: true, manual: true
			};
			if (!providers[provider]) return ctx.util.E.INVALID_PARAMS;
			let protos = { dot: true, doh: true, doq: true, odoh: true, dnscrypt: true };
			if (!protos[proto]) return ctx.util.E.INVALID_PARAMS;

			// DEGRADE: secure resolvers require dnsproxy (DoH/DoT/DoQ/DNSCrypt) or
			// dnscrypt-proxy (ODoH). Reject when the backend is absent; never write
			// the package config files.
			let need = (proto == "odoh") ? "dnscrypt-proxy" : "dnsproxy";
			if (!fs.access("/etc/init.d/" + need))
				return {
					err_code: ERRORS.SECURE_UNAVAILABLE,
					err_msg: "secure DNS (" + proto + ") unavailable: " + need + " not installed"
				};

			// Package present: validate + persist GL state; leave the package to own
			// its upstream config file. dnsmasq is pointed at the local resolver.
			if (provider == "manual") {
				let sml = as_list(args.secure_manual_list);
				if (length(sml) == 0 || sml[0] == "")
					return ctx.util.E.INVALID_PARAMS;
				if (length(sml) > 5)
					return { err_code: ERRORS.PARAMETER_INVALID, err_msg: "exceeds maximum allowed size (5)" };
				c.set(CFG, SEC, "secure_manual_list", sml);
				c.set(CFG, SEC, "provider", "manual");
				c.set(CFG, SEC, "proto", proto);
				c.set(CFG, SEC, "proto_manual", proto);
			} else if (provider == "controld") {
				let ct = int(args.controld_type);
				if (ct != 0 && ct != 1)
					return ctx.util.E.INVALID_PARAMS;
				c.set(CFG, SEC, "controld_type", ct);
				if (ct == 0) {
					let cid = args.controld_id;
					if (type(cid) != "string" || !match(cid, /^[a-zA-Z0-9_-]+$/) ||
					    length(cid) < 2 || length(cid) > 32)
						return { err_code: ERRORS.PARAMETER_INVALID, err_msg: "parameter error" };
					c.set(CFG, SEC, "controld_id", cid);
				} else {
					if (length(as_list(args.servers_list)) == 0)
						return ctx.util.E.INVALID_PARAMS;
					c.set(CFG, SEC, "servers_list", as_list(args.servers_list));
				}
				c.set(CFG, SEC, "provider", "controld");
				c.set(CFG, SEC, "proto", proto);
			} else if (provider == "nextdns") {
				let nid = args.nextdns_id;
				if (type(nid) == "string" && length(nid) > 0 &&
				    !match(nid, /^[a-zA-Z0-9][a-zA-Z0-9-]*$/))
					return { err_code: ERRORS.PARAMETER_INVALID, err_msg: "parameter error" };
				c.set(CFG, SEC, "nextdns_id", nid ?? "");
				c.set(CFG, SEC, "provider", "nextdns");
				c.set(CFG, SEC, "proto", proto);
			} else {
				if (length(as_list(args.servers_list)) == 0)
					return ctx.util.E.INVALID_PARAMS;
				c.set(CFG, SEC, "servers_list", as_list(args.servers_list));
				c.set(CFG, SEC, "provider", provider);
				c.set(CFG, SEC, "proto", proto);
			}

			c.set("dhcp", "@dnsmasq[0]", "server", [ "127.0.0.1#5453" ]);
			c.set("dhcp", "@dnsmasq[0]", "noresolv", "1");
			c.set("dhcp", "@dnsmasq[0]", "localuse", "1");
		}

		if (mode != null)
			c.set(CFG, SEC, "mode", mode);
		c.set("dhcp", "@dnsmasq[0]", "rebind_protection", args.rebind_protection ? "1" : "0");
		c.set(CFG, SEC, "override_vpn", args.override_vpn ? "1" : "0");
		// DEGRADE: vpn_client.set_dns_override_vpn(override_vpn) dropped (GL helper absent).

		// force_dns lives in our store; enforced as a fw4 redirect below.
		c.set(CFG, SEC, "force_dns", args.force_dns ? "1" : "0");
		c.commit(CFG);

		// Secure backend lifecycle (only if the package exists).
		if (mode != null) {
			if (fs.access("/etc/init.d/dnsproxy"))
				ctx.popen("/etc/init.d/dnsproxy stop >/dev/null 2>&1");
			if (fs.access("/etc/init.d/dnscrypt-proxy"))
				ctx.popen("/etc/init.d/dnscrypt-proxy stop >/dev/null 2>&1");
			if (mode == "secure") {
				if (proto == "odoh" && fs.access("/etc/init.d/dnscrypt-proxy"))
					ctx.popen("/etc/init.d/dnscrypt-proxy start >/dev/null 2>&1");
				else if (proto != "odoh" && fs.access("/etc/init.d/dnsproxy"))
					ctx.popen("/etc/init.d/dnsproxy start >/dev/null 2>&1");
			}
		}

		c.commit("dhcp");
		ctx.popen("/etc/init.d/dnsmasq restart >/dev/null 2>&1");

		if (mode != null && fs.access("/etc/init.d/chronyd"))
			ctx.popen("/etc/init.d/chronyd restart >/dev/null 2>&1");

		apply_force_dns(ctx, args.force_dns);
		ctx.popen("sync");
		return {};
	},

	get_host: function(args, ctx) {
		return { content: fs.readfile("/etc/hosts") ?? "" };
	},

	set_host: function(args, ctx) {
		let content = args.content;
		if (type(content) != "string")
			return ctx.util.E.INVALID_PARAMS;
		if (substr(content, length(content) - 1) != "\n")
			content += "\n";
		fs.writefile("/etc/hosts", content);
		ctx.popen("sync");
		ctx.popen("/etc/init.d/dnsmasq restart >/dev/null 2>&1");
		return {};
	}
};
