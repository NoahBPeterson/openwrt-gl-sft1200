// gl-ucode object: ipv6  (port of decompiled-spec/rpc/ipv6.lua)
// IPv6 enable + LAN mode (native/relay/nat6/static) + IPv6 DNS.
// There is no `glipv6` uci on mainline: GL's gl_ipv6 init script translated it
// into stock IPv6, so we do that translation inline against stock `network` lan
// (ip6assign/ip6ifaceid/ip6addr) and `dhcp` lan (dhcpv6/ra/ndp/dns) and reload
// stock netifd+odhcpd. `glipv6` is kept ONLY as our own store for the mode/dns
// flags (mode isn't cleanly derivable from stock); enforcement is always stock.
// DEGRADE/DROP: GL's kmwan, shortcut-fe, zerotier and vpn-client restarts and the
// ovpnserver ifup are gone (none exist on this mainline SF1688 target). nat6 is
// enforced as native prefix-assign; masquerade6 (firewall zone option) is not
// applied here.
'use strict';

// Faithful port of the Lua whitelist idiom `s:gsub("[:%w/%s]+","") == ""`:
// removing every allowed char must leave the empty string.
function whitelist_ok(s) {
	return replace(s, /[:a-zA-Z0-9\/\s]+/g, "") == "";
}

function set_lan_ra(c, ra, dhcpv6, ndp) {
	c.set("dhcp", "lan", "ra", ra);
	c.set("dhcp", "lan", "dhcpv6", dhcpv6);
	c.set("dhcp", "lan", "ndp", ndp);
}

// Translate a GL ipv6 mode into stock network+dhcp options (what gl_ipv6 did).
function apply_stock(c, enable, mode, lan_ip) {
	if (!enable) {
		set_lan_ra(c, "disabled", "disabled", "disabled");
		c.delete("network", "lan", "ip6assign");
		c.delete("network", "lan", "ip6ifaceid");
		c.delete("network", "lan", "ip6addr");
		return;
	}
	if (mode == "native" || mode == "nat6") {
		// nat6 == native prefix-assign here (masquerade6 not enforced; DEGRADE)
		c.delete("network", "lan", "ip6addr");
		c.set("network", "lan", "ip6assign", "60");
		c.set("network", "lan", "ip6ifaceid", "::1");
		set_lan_ra(c, "server", "server", "disabled");
		c.set("dhcp", "lan", "ra_management", "1");
	} else if (mode == "relay") {
		c.delete("network", "lan", "ip6assign");
		c.delete("network", "lan", "ip6ifaceid");
		c.delete("network", "lan", "ip6addr");
		set_lan_ra(c, "relay", "relay", "relay");
		c.set("dhcp", "lan", "master", "0");
		if (c.get("dhcp", "wan6") != null) {
			c.set("dhcp", "wan6", "ra", "relay");
			c.set("dhcp", "wan6", "dhcpv6", "relay");
			c.set("dhcp", "wan6", "ndp", "relay");
			c.set("dhcp", "wan6", "master", "1");
		}
	} else if (mode == "static") {
		c.delete("network", "lan", "ip6assign");
		c.set("network", "lan", "ip6addr", lan_ip);
		c.set("network", "lan", "ip6ifaceid", "::1");
		set_lan_ra(c, "server", "server", "disabled");
	}
}

function reload_stock(ctx) {
	ctx.popen("(/etc/init.d/network reload; /etc/init.d/odhcpd restart) >/dev/null 2>&1 &");
}

export default {
	get_ipv6: function(args, ctx) {
		let c = ctx.uci.cursor();
		// enable derived from stock dhcp lan ra (fall back to our glipv6 store)
		let ra = c.get("dhcp", "lan", "ra");
		let enable = (ra != null) ? (ra != "disabled")
		           : (c.get("glipv6", "globals", "enabled") == "1");
		let lan_mode = c.get("glipv6", "lan", "mode") ?? "native";   // own store
		let lan_ip;
		if (lan_mode == "static")
			lan_ip = c.get("network", "lan", "ip6addr");            // stock
		let lan_dns_mode = c.get("glipv6", "lan", "dnsmode") ?? "auto";
		let lan_dns = c.get("glipv6", "lan", "dns");

		let ipv6 = {
			enable: enable,
			lan_mode: lan_mode,
			lan_ip: lan_ip,
			lan_dns_mode: (lan_dns_mode == "auto")
		};
		if (lan_dns && lan_dns_mode != "auto") {
			if (index(lan_dns, "#") >= 0) {
				let m = match(lan_dns, /([^#]+)#([^#]+)/);
				if (m) {
					ipv6.lan_dns1 = m[1];
					ipv6.lan_dns2 = m[2];
				}
			} else {
				ipv6.lan_dns1 = lan_dns;
				ipv6.lan_dns2 = "";
			}
		}
		if (type(lan_mode) != "string" || lan_dns_mode == null)
			return ctx.util.E.INVALID_PARAMS;
		return ipv6;
	},

	set_ipv6: function(args, ctx) {
		let c = ctx.uci.cursor();
		let enable = args.enable;
		let lan_mode = args.lan_mode;
		let lan_ip = args.lan_ip;
		let lan_dns_mode = args.lan_dns_mode;
		let lan_dns1 = args.lan_dns1;
		let lan_dns2 = args.lan_dns2;
		let lan_dns;

		if (type(enable) == "bool" && enable) {
			// DROP: kmwan member enable + /etc/init.d/kmwan restart (absent target)
			c.set("glipv6", "globals", "enabled", "1");
			c.delete("glipv6", "lan", "interface");

			if (lan_mode == "native") {
				c.set("glipv6", "lan", "mode", "native");
			} else if (lan_mode == "relay") {
				c.set("glipv6", "lan", "mode", "relay");
			} else if (lan_mode == "nat6") {
				c.set("glipv6", "lan", "mode", "nat6");
			} else if (lan_mode == "static") {
				c.set("glipv6", "lan", "mode", "static");
				// preserve the exact static-IP whitelist validation
				if (type(lan_ip) == "string" && whitelist_ok(lan_ip)) {
					c.set("glipv6", "lan", "ip6addr", lan_ip);
				} else {
					return ctx.util.E.INVALID_PARAMS;
				}
			} else {
				return ctx.util.E.INVALID_PARAMS;
			}

			let dns_auto;
			if (type(lan_dns_mode) == "bool" && !lan_dns_mode) {
				c.set("glipv6", "lan", "dnsmode", "manual");
				let dnslist = [];
				if (lan_dns1 && replace(lan_dns1, /\s+/g, "") != "" && whitelist_ok(lan_dns1))
					push(dnslist, lan_dns1);
				if (lan_dns2 && replace(lan_dns2, /\s+/g, "") != "" && whitelist_ok(lan_dns2))
					push(dnslist, lan_dns2);
				if (length(dnslist))
					lan_dns = join("#", dnslist);
				if (lan_dns) {
					c.set("glipv6", "lan", "dns", lan_dns);
					c.set("dhcp", "lan", "dns", dnslist);   // stock RA/DNS
				} else {
					return ctx.util.E.INVALID_PARAMS;
				}
				dns_auto = false;
			} else if (type(lan_dns_mode) == "bool" && lan_dns_mode) {
				c.set("glipv6", "lan", "dnsmode", "auto");
				c.delete("glipv6", "lan", "dns");
				c.delete("dhcp", "lan", "dns");          // stock: advertise self
				dns_auto = true;
			} else {
				return ctx.util.E.INVALID_PARAMS;
			}

			apply_stock(c, true, lan_mode, lan_ip);
			c.commit("glipv6");
			c.commit("network");
			c.commit("dhcp");
			reload_stock(ctx);
		} else {
			// DROP: ovpnserver ifup, kmwan restart, shortcut-fe/zerotier restarts
			c.set("glipv6", "globals", "enabled", "0");
			apply_stock(c, false);
			c.commit("glipv6");
			c.commit("network");
			c.commit("dhcp");
			reload_stock(ctx);
		}
		// DROP: /etc/init.d/vpn-client restart (GL-only, absent on mainline)
		return {};
	}
};
