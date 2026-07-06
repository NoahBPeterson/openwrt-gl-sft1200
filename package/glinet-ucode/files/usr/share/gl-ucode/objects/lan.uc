// gl-ucode object: lan  (port of decompiled-spec/rpc/lan.lua, 4.9.0)
// LAN/Guest/IoT IP + DHCP server + static DHCP reservations. Core IP/DHCP logic
// maps 1:1 to mainline `network` (ipaddr/netmask/device) + `dhcp`
// (ignore/start/limit/leasetime/dhcp_option/host). The Lua `lnet.ipcalc`/
// `bitwise_and` subnet math is reimplemented inline with integer bit ops.
//
// DEGRADE / DROP (vs GL firmware):
//  - gl-black_white_list AP-isolate  -> wireless `isolate` option + immediate
//    /sys/class/net/<ifname>/brport/isolated write (bridge isolation).
//  - gl-black_white_list WAN-isolate -> the object's own standard firewall
//    REJECT rule (src=<net> dest=wan dest_ip=<wan net>), ported verbatim.
//  - guest/iot transfer_enable       -> the object's own `net_secure` firewall
//    zone+forwarding, ported verbatim.
//  - We keep the `gl-black_white_list` uci name as our own read/write store for
//    ap_isolate/wan_isolate/transfer_enable so get/set stay consistent.
//  - DROPPED: gl_timer wifi-cron restarts, lpm change_phy_config, gl-mesh
//    config_renew, TAP-S2S/ovpn HUP branch, MediaTek/QCA wifi kick + hardware
//    switch restart, nginx-cert regeneration + mcu "LAN IP change" message,
//    ap_isolate_config.lua, LAN-port down/up bounce. ngx.timer deferral becomes
//    a synchronous reload (this is a set_* method, so heavy work is allowed).
//  - Guest/IoT network+dhcp sections are absent on stock: every access is
//    existence-guarded and simply skipped when the section does not exist.
'use strict';

const fs = require('fs');

const ERR_NO_LAN = -5;
const WAN_IP_CONFLICT = -7;
const LAN_IP_CONFLICT = -8;
const GUEST_IP_CONFLICT = -9;
const IOT_IP_CONFLICT = -11;

// ---- IPv4 integer math -----------------------------------------------------

function ip2int(ip) {
	let m = match(ip, /^([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)$/);
	if (!m) return null;
	let a = int(m[1]), b = int(m[2]), c = int(m[3]), d = int(m[4]);
	if (a > 255 || b > 255 || c > 255 || d > 255) return null;
	return a * 16777216 + b * 65536 + c * 256 + d;
}

function int2ip(n) {
	return sprintf("%d.%d.%d.%d", (n >> 24) & 255, (n >> 16) & 255, (n >> 8) & 255, n & 255);
}

function mask2prefix(nm) {
	let n = ip2int(nm);
	if (n == null) return null;
	let p = 0;
	for (let i = 31; i >= 0; i--) {
		if (n & (1 << i)) p++;
		else break;
	}
	return p;
}

function prefix2mask(p) {
	p = int(p);
	if (p <= 0) return 0;
	if (p >= 32) return 0xFFFFFFFF;
	return (0xFFFFFFFF << (32 - p)) & 0xFFFFFFFF;
}

// { network: "a.b.c.d", prefix: N, netmask, ipaddr } or null
function ipcalc(ip, netmask) {
	let ipn = ip2int(ip), mn = ip2int(netmask);
	if (ipn == null || mn == null) return null;
	return { network: int2ip(ipn & mn), prefix: mask2prefix(netmask), netmask: netmask, ipaddr: ip };
}

function net_from_cidr(addr, plen) {
	let ipn = ip2int(addr);
	if (ipn == null) return null;
	let mask = prefix2mask(plen);
	return { network: int2ip(ipn & mask), prefix: int(plen) };
}

// port of Lua network_conflict(): the two network addresses share their top
// min(prefix1,prefix2) bits.
function networks_conflict(a, b) {
	if (!a || !b) return false;
	let p = a.prefix < b.prefix ? a.prefix : b.prefix;
	let n1 = ip2int(a.network), n2 = ip2int(b.network);
	if (n1 == null || n2 == null) return false;
	let mask = p <= 0 ? 0 : prefix2mask(p);
	return (n1 & mask) == (n2 & mask);
}

// ---- validators (port of gl.validator) -------------------------------------

function is_ip4(s) { return s != null && ip2int(s) != null; }

function is_netmask4(s) {
	let n = ip2int(s);
	if (n == null) return false;
	let inv = (~n) & 0xFFFFFFFF;
	return (inv & (inv + 1)) == 0;   // inverse must be 0..0 1..1 (contiguous)
}

function is_mac(s) {
	return s != null && match(s, /^[0-9a-fA-F]{2}(:[0-9a-fA-F]{2}){5}$/) != null;
}

// validator.base(val, type, allow_nil, minlen, maxlen)
function v_base(val, expect, allow_nil, minlen, maxlen) {
	if (val == null) return allow_nil ? true : false;
	if (expect == "table") {
		if (type(val) != "array" && type(val) != "object") return false;
	} else if (type(val) != expect) {
		return false;
	}
	if (expect == "string") {
		let L = length(val);
		if (minlen != null && L < minlen) return false;
		if (maxlen != null && L > maxlen) return false;
	}
	return true;
}

function is_private_ip(ip) {
	let m = match(ip, /^([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)$/);
	if (!m) return false;
	let a = int(m[1]), b = int(m[2]);
	if (a == 10) return true;
	if (a == 172 && b >= 16 && b <= 31) return true;
	if (a == 192 && b == 168) return true;
	return false;
}

// ---- dhcp_option parsing (port of get_dhcp_option) -------------------------

function get_dhcp_option(c, iface) {
	let opt = c.get("dhcp", iface, "dhcp_option");
	let force = c.get("dhcp", iface, "dhcp_option_force");
	let gateway = "", dns = [], lpr = [];
	if (type(opt) == "array") {
		for (let v in opt) {
			let head = substr(v, 0, 2);
			if (head == "3,") gateway = substr(v, 2);
			else if (head == "6,") dns = split(substr(v, 2), ",");
		}
	}
	if (type(force) == "array") {
		for (let v in force) {
			if (substr(v, 0, 2) == "9,") lpr = split(substr(v, 2), ",");
		}
	}
	return { gateway: gateway, dns: dns, lpr: lpr };
}

// ---- WAN address discovery (port of get_wan_ipaddr) ------------------------

function get_wan_ipaddr(ctx, c, exclude_modem) {
	let wan_array = ["wan", "wwan", "tethering", "secondwan", "usbwan"];
	if (!exclude_modem) {
		c.foreach("network", "interface", function(s) {
			let name = s[".name"];
			if (substr(name, 0, 6) == "modem_") {
				let proto = c.get("network", name, "proto");
				if (proto && proto != "dhcpv6") {
					if (proto == "qcm" || proto == "qmi") push(wan_array, name + "_4");
					else push(wan_array, name);
				}
			}
		});
	}
	let out = [];
	for (let name in wan_array) {
		let s = ctx.ubus.call("network.interface." + name, "status");
		let a = (s && s["ipv4-address"] && s["ipv4-address"][0]) ? s["ipv4-address"][0] : null;
		if (a && a.address && a.mask != null) {
			// netifd reports mask as a prefix length
			let info = net_from_cidr(a.address, a.mask);
			if (info) push(out, { interface: name, info: info });
		}
	}
	return out;
}

// ---- default DHCP pool (port of calc_defalut_addr) -------------------------

function calc_default_addr(ip, netmask) {
	let info = ipcalc(ip, netmask);
	let prefix = info ? info.prefix : 24;
	let parts = split(ip, ".");
	let base3 = parts[0] + "." + parts[1] + "." + parts[2] + ".";
	let hostmask = [128, 192, 224, 240, 248, 252, 254, 255];
	let available = [128, 64, 32, 16, 8, 4, 2, 1];
	if (prefix > 24) {
		let idx = prefix - 24 - 1;   // 1-based table -> 0-based array
		let start_num = int(parts[3]) & hostmask[idx];
		let s, e;
		if (available[idx] > 2) { s = start_num + 1; e = start_num + available[idx] - 2; }
		else { s = start_num; e = start_num + available[idx] - 1; }
		return [ base3 + s, base3 + e ];
	}
	return [ base3 + "100", base3 + "249" ];
}

// ---- conflict helpers ------------------------------------------------------

function check_wan_conflict(ctx, c, ip_info) {
	for (let addr in get_wan_ipaddr(ctx, c, false))
		if (networks_conflict(addr.info, ip_info)) return true;
	return false;
}

// port of check_network_ip_conflict
function check_network_ip_conflict(c, name, ip_info, lanip) {
	let ipaddr, netmask, disabled;
	if (name == "lan" && lanip) {
		ipaddr = lanip;
		netmask = c.get("network", "lan", "netmask");
		disabled = "0";
	} else {
		ipaddr = c.get("network", name, "ipaddr");
		netmask = c.get("network", name, "netmask");
		disabled = c.get("network", name, "disabled");
	}
	if (ipaddr && netmask) {
		let ninfo = ipcalc(ipaddr, netmask);
		if (ninfo && networks_conflict(ip_info, ninfo) && (disabled == null || disabled == "0"))
			return true;
	}
	return false;
}

// ---- firewall degrades (ported verbatim from the Lua) ----------------------

// port of set_network_wan_isolate: rebuild standard REJECT rules <net> -> wan.
function set_network_wan_isolate(ctx, c, network_name, wan_isolate) {
	let wan_array = ["wan", "wwan", "tethering", "secondwan", "usbwan"];
	for (let name in wan_array)
		c.delete("firewall", name + "_" + network_name + "_isolate");
	if (wan_isolate == 1) {
		for (let addr in get_wan_ipaddr(ctx, c, true)) {
			let wi = addr.info, iface = addr.interface;
			if (wi && iface && is_private_ip(wi.network)) {
				let section = iface + "_" + network_name + "_isolate";
				c.set("firewall", section, "rule");
				c.set("firewall", section, "gl_prio", "2");
				c.set("firewall", section, "src", network_name);
				c.set("firewall", section, "dest", "wan");
				c.set("firewall", section, "proto", "all");
				c.set("firewall", section, "target", "REJECT");
				c.set("firewall", section, "name", "isolate " + network_name + " and " + iface);
				c.set("firewall", section, "dest_ip", wi.network + "/" + wi.prefix);
			}
		}
	}
	c.commit("firewall");
}

// port of set_network_transfer_enable: net_secure zone allows <net> -> lan.
function set_network_transfer_enable(c, iface_store, network, transfer_enable) {
	c.foreach("firewall", "zone", function(s) {
		if (c.get("firewall", s[".name"], "name") == "net_secure") c.delete("firewall", s[".name"]);
	});
	c.foreach("firewall", "forwarding", function(s) {
		if (c.get("firewall", s[".name"], "src") == "net_secure") c.delete("firewall", s[".name"]);
	});
	if (transfer_enable == 1) {
		let zone = c.add("firewall", "zone");
		let fwd = c.add("firewall", "forwarding");
		c.set("firewall", zone, "name", "net_secure");
		c.set("firewall", zone, "input", "ACCEPT");
		c.set("firewall", zone, "forward", "REJECT");
		c.set("firewall", zone, "network", ["lan"]);
		c.set("firewall", zone, "output", "ACCEPT");
		c.set("firewall", fwd, "dest", network);
		c.set("firewall", fwd, "src", "net_secure");
	}
	c.set("gl-black_white_list", iface_store, "transfer_enable", transfer_enable);
}

// ---- per-interface config view (port of get_config_list.get_info) ----------

// Mainline DSA stores the LAN address as `list ipaddr '192.168.8.1/24'` (CIDR in a
// list, no separate `netmask` option); GL's handler expects flat ipaddr + netmask.
// Normalize both forms so ip/netmask (and thus the dhcp start/end math) are correct.
function get_ip_netmask(c, iface) {
	let ip = c.get("network", iface, "ipaddr");
	let netmask = c.get("network", iface, "netmask");
	if (type(ip) == "array") ip = ip[0];
	if (type(ip) == "string") {
		let cm = match(ip, /^([0-9.]+)\/([0-9]+)$/);
		if (cm) {
			ip = cm[1];
			if (netmask == null) netmask = int2ip(prefix2mask(int(cm[2])));
		}
	}
	return { ip: ip, netmask: netmask };
}

function iface_info(ctx, c, iface) {
	let nm = get_ip_netmask(c, iface);
	let ip = nm.ip;
	let netmask = nm.netmask;
	let ignore = c.get("dhcp", iface, "ignore") ?? "0";
	let dstart = c.get("dhcp", iface, "start");
	let dlimit = c.get("dhcp", iface, "limit");
	let start_addr = null, end_addr = null;
	if (ip && netmask) {
		let info = ipcalc(ip, netmask);
		if (info && dstart != null && dlimit != null) {
			let base = ip2int(info.network);
			let so = int(dstart), lim = int(dlimit);
			if (base != null && lim > 0) {
				start_addr = int2ip(base + so);
				end_addr = int2ip(base + so + lim - 1);
			}
		}
	}
	let leasetime = c.get("dhcp", iface, "leasetime");
	let ap_isolate = c.get("gl-black_white_list", iface, "ap_isolate");
	let wan_isolate = c.get("gl-black_white_list", iface, "wan_isolate");
	let opt = get_dhcp_option(c, iface);
	let enable = (ignore == "0") ? 1 : 0;

	if (iface == "guest" && enable == 1) {
		let g2 = c.get("wireless", "guest2g", "disabled") ?? "0";
		let g5 = c.get("wireless", "guest5g", "disabled") ?? "0";
		let g6 = "1", gmlo = "1";
		if (c.get("wireless", "guest6g") != null) g6 = c.get("wireless", "guest6g", "disabled") ?? "0";
		if (c.get("mlo", "mld1") != null) gmlo = c.get("mlo", "mld1", "disabled") ?? "0";
		if (g2 == "1" && g5 == "1" && g6 == "1" && gmlo == "1") enable = 0;
	}
	if (iface == "iot" && enable == 1) {
		let i2 = c.get("wireless", "iot2g", "disabled") ?? "0";
		let i5 = c.get("wireless", "iot5g", "disabled") ?? "0";
		if (i2 == "1" && i5 == "1") enable = 0;
	}

	return {
		interface: iface,
		ip: ip,
		netmask: netmask,
		enable: enable,
		start: start_addr,
		"end": end_addr,
		gateway: opt.gateway,
		leasetime: leasetime,
		dns: opt.dns,
		lpr: opt.lpr,
		ap_isolate: ap_isolate != null ? int(ap_isolate) : null,
		wan_isolate: wan_isolate != null ? int(wan_isolate) : null
	};
}

export default {
	get_config_list: function(args, ctx) {
		let c = ctx.uci.cursor();
		let s = ctx.ubus.call("network.interface.lan", "status");
		if (!s || !s.up)
			return { err_code: ERR_NO_LAN, err_msg: "no lan interface" };

		let interfaces = [ iface_info(ctx, c, "lan") ];
		if (c.get("network", "guest") != null) {
			let te = c.get("gl-black_white_list", "guest", "transfer_enable");
			let info = iface_info(ctx, c, "guest");
			info.transfer_enable = te != null ? int(te) : null;
			push(interfaces, info);
		}
		if (c.get("network", "iot") != null) {
			let te = c.get("gl-black_white_list", "iot", "transfer_enable");
			let info = iface_info(ctx, c, "iot");
			info.transfer_enable = te != null ? int(te) : null;
			push(interfaces, info);
		}
		return { interfaces: interfaces };
	},

	set_config: function(params, ctx) {
		let c = ctx.uci.cursor();
		let E = ctx.util.E;
		let interface = params.interface;

		if (interface != "lan" && interface != "guest" && interface != "iot")
			return E.INVALID_PARAMS;

		let ip = params.ip ?? c.get("network", interface, "ipaddr");
		let netmask = params.netmask ?? c.get("network", interface, "netmask");
		let ap_isolate = params.ap_isolate;
		let transfer_enable = params.transfer_enable;
		let lanip = c.get("network", "lan", "ipaddr");
		// DROPPED: TAP-S2S ovpnclient guard (no OpenVPN TAP-S2S on this device)

		if (params.netmask != null && !is_netmask4(params.netmask)) return E.INVALID_PARAMS;
		if (params.ip != null && !is_ip4(params.ip)) return E.INVALID_PARAMS;
		if (!is_ip4(ip) || !is_netmask4(netmask)) return E.INVALID_PARAMS;

		let cur_opt = get_dhcp_option(c, interface);
		let gateway = params.gateway;
		if (gateway == null) gateway = cur_opt.gateway;
		else if (gateway != "" && !is_ip4(gateway)) return E.INVALID_PARAMS;

		let start_addr = params.start ?? "";
		if (params.start != null && params.start != "" && !is_ip4(params.start)) return E.INVALID_PARAMS;
		let end_addr = params["end"] ?? "";
		if (params["end"] != null && params["end"] != "" && !is_ip4(params["end"])) return E.INVALID_PARAMS;

		if (start_addr != "" && end_addr != "") {
			let laninfo = ipcalc(ip, netmask);
			let startinfo = ipcalc(start_addr, netmask);
			let endinfo = ipcalc(end_addr, netmask);
			if (start_addr == end_addr) return E.INVALID_PARAMS;
			if (ip2int(start_addr) > ip2int(end_addr)) return E.INVALID_PARAMS;
			if (!laninfo || !startinfo || !endinfo ||
			    startinfo.network != laninfo.network || endinfo.network != laninfo.network)
				return E.INVALID_PARAMS;
		}

		let use_new_addrpool = false;
		if (params.start != null ||
		    (params.netmask != null && params.netmask != c.get("network", interface, "netmask")))
			use_new_addrpool = true;

		if (start_addr == "" && end_addr == "") {
			let def = calc_default_addr(ip, netmask);
			start_addr = def[0];
			end_addr = def[1];
		}

		let leasetime = params.leasetime ?? c.get("dhcp", interface, "leasetime") ?? "12h";
		if (!v_base(params.leasetime, "string", true, 1, 16)) return E.INVALID_PARAMS;

		let lpr = params.lpr;
		if (!v_base(lpr, "table", true)) return E.INVALID_PARAMS;
		else if (lpr == null) lpr = cur_opt.lpr;
		else if (lpr[0] != "") {
			for (let lpr_ip in lpr) if (!is_ip4(lpr_ip)) return E.INVALID_PARAMS;
		}

		let dns = params.dns;
		if (!v_base(dns, "table", true)) return E.INVALID_PARAMS;
		else if (dns == null) dns = cur_opt.dns;
		else for (let dns_ip in dns) if (!is_ip4(dns_ip)) return E.INVALID_PARAMS;

		if (params.enable != null && params.enable != 0 && params.enable != 1) return E.INVALID_PARAMS;
		if (ap_isolate != null && ap_isolate != 0 && ap_isolate != 1) return E.INVALID_PARAMS;
		if (params.wan_isolate != null && params.wan_isolate != 0 && params.wan_isolate != 1) return E.INVALID_PARAMS;
		if (transfer_enable != null && transfer_enable != 0 && transfer_enable != 1) return E.INVALID_PARAMS;

		let dhcp_sw;
		if (params.ip != null || params.netmask != null)
			dhcp_sw = 1 - int(c.get("dhcp", interface, "ignore") ?? "0");
		else
			dhcp_sw = params.enable ?? 1;

		let ip_info = ipcalc(ip, netmask);   // network/prefix for conflict math
		if (!ip_info) return E.INVALID_PARAMS;

		// conflict checks
		if (check_wan_conflict(ctx, c, ip_info))
			return { err_code: WAN_IP_CONFLICT, err_msg: "conflict with wan ip" };

		if (interface == "lan") {
			if (check_network_ip_conflict(c, "guest", ip_info))
				return { err_code: GUEST_IP_CONFLICT, err_msg: "conflict with guest ip" };
			if (check_network_ip_conflict(c, "iot", ip_info))
				return { err_code: IOT_IP_CONFLICT, err_msg: "conflict with iot ip" };
		} else if (interface == "iot") {
			if (check_network_ip_conflict(c, "lan", ip_info, lanip))
				return { err_code: LAN_IP_CONFLICT, err_msg: "conflict with lan ip" };
			if (check_network_ip_conflict(c, "guest", ip_info))
				return { err_code: GUEST_IP_CONFLICT, err_msg: "conflict with guest ip" };
		} else {
			if (check_network_ip_conflict(c, "lan", ip_info, lanip))
				return { err_code: LAN_IP_CONFLICT, err_msg: "conflict with lan ip" };
			if (check_network_ip_conflict(c, "iot", ip_info))
				return { err_code: IOT_IP_CONFLICT, err_msg: "conflict with iot ip" };
		}

		// ---- commit network ip/netmask ----
		c.set("network", interface, "ipaddr", ip);
		c.set("network", interface, "netmask", netmask);
		// DROPPED: gl_timer wifi-cron restarts, lpm change_phy_config,
		//          ap_isolate_config.lua invocation.

		// AP isolation degrade: persist a wireless `isolate` option on this
		// network's APs, and keep the value in our gl-black_white_list store.
		if (ap_isolate != null) {
			c.foreach("wireless", "wifi-iface", function(sec) {
				if (c.get("wireless", sec[".name"], "network") == interface)
					c.set("wireless", sec[".name"], "isolate", sprintf("%d", int(ap_isolate)));
			});
			c.set("gl-black_white_list", interface, "ap_isolate", sprintf("%d", int(ap_isolate)));
			c.commit("wireless");
			c.commit("gl-black_white_list");
		}

		if ((interface == "guest" || interface == "iot") && params.wan_isolate != null) {
			set_network_wan_isolate(ctx, c, interface, int(params.wan_isolate));
			c.set("gl-black_white_list", interface, "wan_isolate", sprintf("%d", int(params.wan_isolate)));
			c.commit("gl-black_white_list");
		}
		if ((interface == "guest" || interface == "iot") && transfer_enable != null) {
			set_network_transfer_enable(c, interface, interface, int(transfer_enable));
			c.commit("gl-black_white_list");
		}

		// ---- DHCP pool (start offset / limit) ----
		if (use_new_addrpool) {
			let base = ip2int(ip_info.network);
			let sn = ip2int(start_addr), en = ip2int(end_addr);
			if (base != null && sn != null && en != null) {
				let start_off = sn - base;
				let pool_limit = en - sn + 1;
				if (start_off > 0) {
					c.set("dhcp", interface, "start", sprintf("%d", start_off));
					c.set("dhcp", interface, "limit", sprintf("%d", pool_limit));
				}
			}
		}

		c.set("dhcp", interface, "interface", interface);
		// GL-specific dhcp/firewall sections: only touch when they exist (DEGRADE)
		if (c.get("dhcp", "localhost") != null) c.set("dhcp", "localhost", "ip", ip);
		if (c.get("firewall", "dns_over_" + interface) != null)
			c.set("firewall", "dns_over_" + interface, "dest_ip", ip);

		if (dhcp_sw == 1) {
			c.set("dhcp", interface, "ignore", "0");
			if (interface == "lan") {
				if (c.get("dhcp", "@domain[0]") != null) c.set("dhcp", "@domain[0]", "ip", ip);
				if (c.get("dhcp", "@domain[1]") != null) c.set("dhcp", "@domain[1]", "ip", "::ffff:" + ip);
			}
			c.delete("dhcp", interface, "dhcp_option");
			c.delete("dhcp", interface, "dhcp_option_force");
			let option_info = [];
			if (gateway != "") push(option_info, "3," + gateway);
			if (length(dns) != 0) push(option_info, "6," + join(",", dns));
			let lpr_args = [];
			for (let v in lpr) if (v != "") push(lpr_args, v);
			if (length(lpr_args) >= 1)
				c.set("dhcp", interface, "dhcp_option_force", ["9," + join(",", lpr_args)]);
			if (leasetime != "") c.set("dhcp", interface, "leasetime", leasetime);
			if (length(option_info) != 0) c.set("dhcp", interface, "dhcp_option", option_info);
		} else {
			c.set("dhcp", interface, "ignore", "1");
		}

		c.commit("network");
		c.commit("dhcp");
		c.commit("firewall");
		// DROPPED: nginx cert regen, webdav pem, mcu "LAN IP change" message.

		// ---- apply (synchronous; ngx.timer deferral dropped) ----
		ctx.ubus.call("network", "reload");
		ctx.popen("/etc/init.d/firewall reload >/dev/null 2>&1");
		ctx.popen("/etc/init.d/dnsmasq reload >/dev/null 2>&1");
		// bridge-port isolation applied immediately (DEGRADE of the ecm branch)
		if (ap_isolate != null) {
			c.foreach("wireless", "wifi-iface", function(sec) {
				if (c.get("wireless", sec[".name"], "network") == interface) {
					let ifn = c.get("wireless", sec[".name"], "ifname");
					if (ifn)
						ctx.popen(sprintf("echo %d > /sys/class/net/%s/brport/isolated 2>/dev/null",
							int(ap_isolate), ifn));
				}
			});
		}
		ctx.popen("sync 2>/dev/null");
		return {};
	},

	add_static_bind: function(params, ctx) {
		let E = ctx.util.E;
		let mac = params.mac;
		let ip = params.ip;
		let name = params.name;
		let hostname = params.hostname;
		let one_click = params.one_click;

		if (!is_mac(mac)) return E.INVALID_PARAMS;
		if (!is_ip4(ip)) return E.INVALID_PARAMS;
		if (!v_base(name, "string", true, 0, 256) ||
		    (name && name != "" && (index(name, '"') >= 0 || index(name, ",") >= 0)))
			return E.INVALID_PARAMS;
		if (one_click != null && type(one_click) != "bool") return E.INVALID_PARAMS;
		if (!one_click &&
		    (!v_base(hostname, "string", true, 0, 63) ||
		     (hostname && hostname != "" &&
		      (!match(hostname, /^[a-zA-Z0-9][a-zA-Z0-9-]*[a-zA-Z0-9]$/) || match(hostname, /^[0-9]+$/)))))
			return E.INVALID_PARAMS;

		if (!name || name == "") name = "";
		if (!hostname || hostname == "") hostname = "";

		let c = ctx.uci.cursor();
		let conflict = false, conflict_mac = "";
		c.foreach("dhcp", "host", function(s) {
			if (s.mac == mac) c.delete("dhcp", s[".name"]);
			if (s.ip == ip && s.mac != mac) c.delete("dhcp", s[".name"]);
			if (!one_click && hostname != "" && s.name == hostname) {
				conflict = true;
				conflict_mac = s.mac;
			}
		});
		if (!one_click && hostname != "" && conflict && conflict_mac != mac)
			return E.INVALID_PARAMS;

		// best-effort client alias (gl-session absent on mainline -> ignored)
		ctx.ubus.call("gl-session", "call",
			{ module: "clients", func: "set_info", params: { mac: mac, alias: name } });

		let sid = c.add("dhcp", "host");
		c.set("dhcp", sid, "mac", mac);
		c.set("dhcp", sid, "ip", ip);
		c.set("dhcp", sid, "tag", name);
		if (!one_click && hostname != "") c.set("dhcp", sid, "name", hostname);
		c.commit("dhcp");
		ctx.popen("sync 2>/dev/null");
		ctx.popen("/etc/init.d/dnsmasq restart >/dev/null 2>&1");
		return {};
	},

	set_static_bind: function(params, ctx) {
		return this.add_static_bind(params, ctx);
	},

	get_static_bind_list: function(args, ctx) {
		let c = ctx.uci.cursor();
		let res = [];
		c.foreach("dhcp", "host", function(s) {
			if (type(s.mac) == "array" || type(s.tag) == "array") return;
			push(res, {
				mac: s.mac,
				ip: s.ip,
				name: (s.name != null && s.name != "") ? s.name : null,
				tag: (s.tag != null && s.tag != "") ? s.tag : null
			});
		});
		return { static_bind_list: res };
	},

	remove_static_bind: function(params, ctx) {
		let E = ctx.util.E;
		let c = ctx.uci.cursor();
		let mode = params.mode;
		if (mode != 0 && mode != 1) return E.INVALID_PARAMS;
		if (mode == 0 && !is_mac(params.mac)) return E.INVALID_PARAMS;
		c.foreach("dhcp", "host", function(s) {
			if (mode == 1 || s.mac == params.mac) c.delete("dhcp", s[".name"]);
		});
		c.commit("dhcp");
		ctx.popen("/etc/init.d/dnsmasq restart >/dev/null 2>&1");
		ctx.popen("sync 2>/dev/null");
		return {};
	},

	get_wan_info: function(params, ctx) {
		let c = ctx.uci.cursor();
		return { wan_info: get_wan_ipaddr(ctx, c, true) };
	}
};
