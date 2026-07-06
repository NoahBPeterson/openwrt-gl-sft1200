// gl-ucode object: cable  (port of decompiled-spec/rpc/cable.lua, 4.9.0)
// Wired WAN/LAN port setup + status (proto dhcp/static/pppoe, VLAN via
// network.device, mtu, TTL via nftables/fw4). The GL hardware-abstraction layer is
// DEGRADED to mainline sources on the Siflower SF1688 / SFT1200:
//   - lfactory.check_*_cable -> /sys/class/net/<dev>/carrier + ubus network.device status
//   - lfactory.get_wan_port / gl-eth-ports.json / /etc/board.json switch roles ->
//     a STATIC SFT1200 board map (wan=eth0.2, lan ports lan1/lan2, switch mxl-gsw1xx)
//   - glconfig wan2lan/lan2wan port roles kept as our own uci store (read w/ default)
//   - hardware.platform_vendor_* / platform_get_wan_status -> always write plain uci
//   - kmwan.get_ifstatus -> netifd network.interface.wan status (up + default route)
//   - 2nd-WAN / USB-WAN branches -> stubs (get_secondwan_port/get_usbwan_port = null;
//     this board is single-WAN, so those methods report "Unsupported features!")
//   - MTK/QCA swconfig switch_vlan path DROPPED; only the network.device VLAN path is kept
'use strict';

const fs = require('fs');

function readfile(p) { let d = fs.readfile(p); return d ? trim(d) : null; }

// ---- static SFT1200 board map (DEGRADE: gl-eth-ports.json / board.json) -----------
const LAN_PORTS = [
	{ name: "lan1", device: "lan1", silk: "LAN1" },
	{ name: "lan2", device: "lan2", silk: "LAN2" }
];

const netmodes = { wan: 0, lan: 1, bridge: 2, drop_in: 3, passthrough: 4 };

// wan network device (uci-derived, static fallback eth0.2). Also the "is WAN
// supported" truthiness check (always truthy here => single-WAN board).
function get_interface_dev(c, interface) {
	let use_device = c.get("network", "lan", "device");
	return c.get("network", interface, use_device ? "device" : "ifname");
}

function wan_port(ctx) {
	let c = ctx.uci.cursor();
	return get_interface_dev(c, "wan") ?? "eth0.2";
}

// DEGRADE: 2nd-WAN / USB-WAN absent on this board -> stubs.
function secondwan_port(ctx) { return null; }
function usbwan_port(ctx) { return null; }

function iface_port(ctx, interface) {
	if (interface == "wan") return wan_port(ctx);
	if (interface == "usbwan") return usbwan_port(ctx);
	if (interface == "secondwan") return secondwan_port(ctx);
	return null;
}

// ---- port role (our glconfig store, degrade to physical default) ------------------
function get_port_mode(ctx, name) {
	let c = ctx.uci.cursor();
	if (name == "wan")
		return (c.get("glconfig", "general", "wan2lan") == "1") ? "lan" : "wan";
	if (name == "secondwan")
		return (c.get("glconfig", "general", "lan2wan") == "1") ? "wan" : "lan";
	if (name == "usbwan")
		return (c.get("glconfig", "general", "usb_lan2wan") == "1") ? "wan" : "lan";
	return "wan";
}

function check_router_netmode(ctx) {
	let c = ctx.uci.cursor();
	let mode = c.get("glconfig", "general", "mode");
	if (!mode) return null;
	if (mode == "wds" || mode == "relay" || mode == "ap") return 2;
	if (c.get("edgerouter", "global", "enabled") == "1") return 3;
	if (mode == "passthrough") return 4;
	return null;
}

// ---- cable/carrier link detection (DEGRADE: lfactory.check_*_cable) ----------------
function port_carrier(ctx, dev) {
	if (!dev) return false;
	let car = readfile(`/sys/class/net/${dev}/carrier`);
	if (car != null) return car == "1";
	let s = ctx.ubus.call("network.device", "status", { name: dev });
	return (s && s.carrier) ? true : false;
}

function check_cable(ctx, interface) {
	// secondwan/usbwan are stubbed (no port) -> no cable
	if (interface != "wan") return false;
	for (let cand in [wan_port(ctx), "eth0.2", "wan"])
		if (cand && port_carrier(ctx, cand)) return true;
	return false;
}

// DEGRADE: kmwan.get_ifstatus -> netifd up + default route present.
function iface_online(ctx, ifn) {
	let s = ctx.ubus.call(`network.interface.${ifn}`, "status");
	if (!s || !s.up) return false;
	for (let r in (s.route ?? [])) if (r.target == "0.0.0.0") return true;
	return false;
}

// DEGRADE: lpm-monitor absent on this board -> guarded no-op.
function check_lpm_wakeup(ctx) {
	if (fs.access("/proc/gl-hw-info/lpm-monitor"))
		ctx.ubus.call("lpm", "change_phy_config");
}

function check_wan_supported(ctx, c, interface) {
	if (!iface_port(ctx, interface))
		return { err_code: -4, err_msg: "Unsupported features!" };
	if (!c.get("network", interface))
		return { err_code: -5, err_msg: "no wan interface" };
	return null;
}

// ---- default WAN MAC (DEGRADE: board.network.wan.macaddr / lfactory.get_mac) -------
function wan_defmac(ctx) {
	let c = ctx.uci.cursor();
	let m = c.get("network", "wan", "macaddr");
	if (m) return m;
	for (let dev in [wan_port(ctx), "eth0.2", "eth0"]) {
		let a = readfile(`/sys/class/net/${dev}/address`);
		if (a) return lc(a);
	}
	return "";
}

function hexval(s) {
	let n = 0; s = lc(s ?? "");
	for (let i = 0; i < length(s); i++) {
		let d = index("0123456789abcdef", substr(s, i, 1));
		if (d >= 0) n = n * 16 + d;
	}
	return n;
}

function mac_increase(mac, inc) {
	if (!mac) return mac;
	let head = substr(mac, 0, 8);
	let tail = replace(substr(mac, 9), ":", "");
	let hexs = sprintf("%06x", hexval(tail) + inc);
	let out = "";
	for (let i = 0; i < length(hexs); i += 2) out += ":" + substr(hexs, i, 2);
	return head + out;
}

// ---- VLAN via network.device (DEGRADE: swconfig switch_vlan path dropped) ----------
function find_device_sid(c, name) {
	let sid = null;
	c.foreach("network", "device", function(s) { if (s.name == name) sid = s[".name"]; });
	return sid;
}

function get_interface_vlan(c, interface) {
	let dev = get_interface_dev(c, interface);
	if (!dev) return {};
	let m = match(dev, /\.([0-9]+)/);
	if (m) return { vlan: int(m[1]), tag: true };
	return {};
}

function set_interface_dev(c, interface, ndev) {
	let use_device = c.get("network", "lan", "device");
	let odev = c.get("network", interface, use_device ? "device" : "ifname");
	c.foreach("network", "device", function(s) {
		if (s.name == odev) c.set("network", s[".name"], "name", ndev);
	});
	c.set("network", interface, use_device ? "device" : "ifname", ndev);
	c.set("board_special", "hardware", interface, ndev);
}

function del_interface_dev(c, interface) {
	let use_device = c.get("network", "lan", "device");
	let dev = c.get("network", interface, use_device ? "device" : "ifname");
	c.delete("network", interface, use_device ? "device" : "ifname");
	c.foreach("network", "device", function(s) {
		if (s.name == dev) c.delete("network", s[".name"]);
	});
}

function check_vlan_conflict(c, interface, vlan) {
	let conflict = false;
	for (let name in ["wan", "secondwan", "usbwan"]) {
		if (name == interface) continue;
		let other = get_interface_vlan(c, name);
		if (other && other.vlan == vlan && other.tag) conflict = true;
	}
	return !conflict;
}

function set_interface_vlan(c, interface, vlan, tag) {
	if (tag && !check_vlan_conflict(c, interface, vlan)) return false;
	let base = match(get_interface_dev(c, interface) ?? "eth0.2", /^([0-9a-zA-Z]+)/);
	let ndev = base ? base[1] : "eth0";
	if (tag) ndev = `${ndev}.${vlan}`;
	set_interface_dev(c, interface, ndev);
	return true;
}

// ---- per-interface status (internal) ----------------------------------------------
function iface_status(ctx, c, interface) {
	let err = check_wan_supported(ctx, c, interface);
	if (err) return err;

	let res = { mode: netmodes[get_port_mode(ctx, interface)] };
	res.status = check_cable(ctx, interface) ? 1 : 3;

	if (interface == "wan") {
		let m = check_router_netmode(ctx);
		if (m != null) res.mode = m;
	}
	if (res.status == 3) return res;

	let mode = c.get("glconfig", "general", "mode");
	let status;
	if (mode == "ap") status = ctx.ubus.call("network.interface.lan", "status");
	else status = ctx.ubus.call(`network.interface.${interface}`, "status");

	res.protocol = (mode == "ap") ? c.get("network", "lan", "proto")
	                              : c.get("network", interface, "proto");
	status = status ?? {};

	if (!status.up) {
		res.status = status.pending ? 2 : 0;
		if (res.protocol == "pppoe") {
			res.username = c.get("network", interface, "username");
			res.password = c.get("network", interface, "password");
			let errors = status.errors;
			let err_code = (errors && length(errors) > 0) ? errors[0].code : null;
			if (err_code == "AUTH_TOPEER_FAILED") {
				res.log = "Invalid Username or Password! Please check and try again.";
			} else if (err_code == "USER_REQUEST") {
				res.log = "Dialing, please wait...";
			} else {
				let data = ctx.popen("logread -e 'pppd'|tail -6|awk -F']:' '{print $2}'");
				if (data) {
					if (index(data, "Timeout waiting for PADO packets") >= 0)
						res.log = "Oops! Cannot detect services. \\n\n" +
						          "If you are unable to dial for a long time and try many times have still not been successful. Please try the following methods:\\n\n" +
						          "1.Check if the Internet cable connection is normal.\\n\"\n" +
						          "2.Connect your PC to the ISP modem and make the PPPoE dialing for testing.\\n\n" +
						          "You shall contact your Internet Services Provider for help after both methods failed.\n";
					else
						res.log = data;
				}
			}
		}
		return res;
	}

	res.status = 1;
	let a4 = status["ipv4-address"];
	if (a4 && a4[0]) {
		let ipv4 = { ip: `${a4[0].address}/${a4[0].mask}` };
		for (let route in (status.route ?? [])) {
			if (route.target == "0.0.0.0") { ipv4.gateway = route.nexthop; break; }
		}
		ipv4.dns = status["dns-server"];
		res.ipv4 = ipv4;
	}

	if (mode != "ap") {
		let s6 = ctx.ubus.call(`network.interface.${interface}6`, "status") ?? {};
		if (s6.up) {
			let ipv6 = [];
			let routes = {};
			for (let route in (s6.route ?? []))
				if (route.target == "::") routes[route.source] = route.nexthop;
			for (let addr in (s6["ipv6-address"] ?? [])) {
				let ip = `${addr.address}/${addr.mask}`;
				let gateway = routes[ip] ?? routes["::/0"];
				push(ipv6, { ip: ip, gateway: gateway, dns: s6["dns-server"] });
			}
			if (length(ipv6) > 0) res.ipv6 = ipv6;
		}
	}
	// DEGRADE: passthrough subsystem absent -> null.
	res.passthrough = ctx.ubus.call("passthrough", "status", { type: "cable" });
	return res;
}

// ---- MAC parse/apply --------------------------------------------------------------
function parse_mac_config(cfg, mode, macaddr) {
	mode = mode ?? "";
	let m = substr(mode, 0, 1);
	if (m == "r") {
		cfg.mode = "random";
		if (substr(mode, 1, 1) == "r") {
			cfg.update = "reboot";
		} else {
			let pm = match(mode, /r([0-9]+)/);
			if (pm) { cfg.update = "time"; cfg.period = int(pm[1]) / 60; }
			else cfg.update = "none";
		}
	} else if (m == "c") {
		cfg.mode = "clone";
	} else {
		cfg.mode = "default";
	}
	cfg.macaddr = macaddr;
}

function get_port_mac_config(ctx, iface, port) {
	if (iface.mode == "lan") return;
	let c = ctx.uci.cursor();
	let found = false;
	c.foreach("network", "device", function(s) {
		if (s.name == port) { found = true; parse_mac_config(iface.macaddr, s.mac_mode, s.macaddr); }
	});
	if (!found)
		parse_mac_config(iface.macaddr, c.get("network", "wan", "mac_mode"),
		                 c.get("network", "wan", "macaddr") ?? wan_defmac(ctx));
}

function set_iface_mac(c, macaddr, port) {
	let need_reload = false, not_device = true, mac_mode = "", mac_expire = "";
	if (macaddr.mode == "random") {
		mac_mode = "r";
		if (macaddr.update == "reboot") mac_mode = "rr";
		else if (macaddr.update == "time") {
			mac_mode = "r" + (macaddr.period * 60);
			mac_expire = time() + macaddr.period * 60 * 60;
		}
	} else if (macaddr.mode == "clone") {
		mac_mode = "c";
	}
	c.foreach("network", "device", function(s) {
		if (s.name != port) return;
		not_device = false;
		c.set("network", s[".name"], "mac_mode", mac_mode);
		c.set("network", s[".name"], "mac_expire", `${mac_expire}`);
		if (s.macaddr != macaddr.macaddr) {
			need_reload = true;
			c.set("network", s[".name"], "macaddr", macaddr.macaddr);
		}
	});
	if (not_device) {
		let old = c.get("network", "wan", "macaddr");
		c.set("network", "wan", "mac_mode", mac_mode);
		c.set("network", "wan", "mac_expire", `${mac_expire}`);
		if (!old || old != macaddr.macaddr) {
			need_reload = true;
			c.set("network", "wan", "macaddr", macaddr.macaddr);
		}
	}
	return need_reload;
}

function is_lan_port(c, port) {
	if (!port) return false;
	let lan = c.get("board_special", "network", "lan");
	if (type(lan) == "string") return index(lan, port) >= 0;
	if (type(lan) == "array") { for (let n in lan) if (n == port) return true; return false; }
	return false;
}

// ---- IPv6 route cleanup / accept_ra (DEGRADE: glipv6 gating) -----------------------
function valid_netdev(device) {
	return device && match(device, /^[0-9a-zA-Z._:-]+$/) != null;
}

function clear_ipv6_routes_by_device(ctx, device) {
	if (!valid_netdev(device)) return;
	let routes = ctx.popen("ip -6 route");
	if (type(routes) != "string") return;
	for (let route in split(routes, "\n")) {
		if (index(route, `dev ${device} `) >= 0 &&
		    (index(route, " proto static ") >= 0 || index(route, " proto ra ") >= 0)) {
			let m = match(route, /^(.*)\smetric/) ?? match(route, /^(.*)\sexpires/) ??
			        match(route, /^(.*)\sweight/);
			let tmp = m ? m[1] : route;
			ctx.popen(`ip -6 route delete ${tmp} 2>/dev/null`);
		}
	}
}

function set_accept_ra_for_wan(ctx, proto, device) {
	let c = ctx.uci.cursor();
	// DEGRADE: glipv6 absent on mainline -> feature disabled, no-op.
	let ipv6_enabled = c.get("glipv6", "globals", "enabled") == "1";
	if (!device || !ipv6_enabled) return;
	let accept_ra_path = `/proc/sys/net/ipv6/conf/${device}/accept_ra`;
	let sysctl = "/etc/sysctl.d/99-disable-ipv6.conf";
	let target = `net.ipv6.conf.${device}.accept_ra=2`;
	if (proto == "pppoe") {
		fs.writefile(accept_ra_path, "0\n");
		ctx.popen(`sed -i '/net.ipv6.conf.${device}.accept_ra=2/d' ${sysctl} 2>/dev/null`);
		let routes = ctx.popen("ip -6 route");
		if (type(routes) != "string") return;
		for (let route in split(routes, "\n")) {
			if (index(route, `dev ${device} `) >= 0) {
				let m = match(route, /^(.*)\smetric/) ?? match(route, /^(.*)\sweight/);
				if (m) ctx.popen(`ip -6 route delete ${m[1]} 2>/dev/null`);
			}
		}
	} else {
		fs.writefile(accept_ra_path, "2\n");
		let cur = readfile(sysctl) ?? "";
		if (index(cur, target) >= 0) return;
		fs.writefile(sysctl, cur + (cur && substr(cur, length(cur) - 1) != "\n" ? "\n" : "") + target + "\n");
	}
}

// ---- port-mode (wan<->lan) switching, device-based (DEGRADE: no swconfig) ----------
function string_split(s, delim) {
	let out = [];
	for (let chunk in split(s ?? "", delim)) if (chunk != "") push(out, chunk);
	return out;
}

function array_remove(a, e) {
	if (!a) return a;
	let out = [];
	for (let x in a) if (x != e) push(out, x);
	return out;
}

function set_port_mode_noswitch(c, ctx, interface, mode) {
	let use_device = c.get("network", "lan", "device");
	let ifname = iface_port(ctx, interface) ?? "eth0.2";
	if (mode == "wan") {
		if (use_device) {
			c.foreach("network", "device", function(s) {
				if (s.name != "br-lan" || !s.ports) return;
				let ports = (type(s.ports) == "string") ? [s.ports] : s.ports;
				ports = array_remove(ports, ifname);
				if (length(ports) > 0) c.set("network", s[".name"], "ports", ports);
				else c.delete("network", s[".name"], "ports");
			});
			c.set("network", interface, "device", ifname);
		} else {
			let ifnames = array_remove(string_split(c.get("network", "lan", "ifname"), " "), ifname);
			c.set("network", "lan", "ifname", join(" ", ifnames));
			c.set("network", interface, "ifname", ifname);
		}
	} else {
		set_interface_vlan(c, interface, 0, false);
		c.delete("board_special", "hardware", interface);
		c.commit("board_special");
		if (use_device) {
			c.foreach("network", "device", function(s) {
				if (s.name != "br-lan") return;
				let ports = s.ports ?? [];
				if (type(ports) == "string") ports = [ports];
				push(ports, ifname);
				c.set("network", s[".name"], "ports", ports);
			});
			c.delete("network", interface, "device");
		} else {
			let ifnames = c.get("network", "lan", "ifname");
			c.set("network", "lan", "ifname", ifnames ? `${ifnames} ${ifname}` : ifname);
			c.delete("network", interface, "ifname");
		}
	}
}

function set_port_mode(ctx, interface, mode, port, route_cleanup_device) {
	if (mode == "lan" && check_cable(ctx, interface))
		return { err_code: -3, err_msg: "need remove internet cable" };

	let c = ctx.uci.cursor();
	let need_reload = false;
	if (interface == "wan") c.set("glconfig", "general", "wan2lan", mode == "wan" ? 0 : 1);
	else if (interface == "secondwan") c.set("glconfig", "general", "lan2wan", mode == "wan" ? 1 : 0);
	else c.set("glconfig", "general", "usb_lan2wan", mode == "wan" ? 1 : 0);
	c.commit("glconfig");

	let interface6 = interface + "6";
	if (mode == "wan") {
		let use_device = c.get("network", "lan", "device");
		c.set("network", interface, "interface");
		c.set("network", interface, "proto", "dhcp");
		c.delete("network", interface, "disabled");
		c.set("network", interface6, "interface");
		c.set("network", interface6, "proto", "dhcpv6");
		if (c.get("glipv6", "globals", "enabled") == "1") c.delete("network", interface6, "disabled");
		else c.set("network", interface6, "disabled", "1");
		if (use_device) c.set("network", interface6, "device", "@" + interface);
		else c.set("network", interface6, "ifname", "@" + interface);
	} else {
		clear_ipv6_routes_by_device(ctx, route_cleanup_device ?? port);
		// DEGRADE: default MAC restore (board.network.wan.macaddr).
		let macaddr = { mode: "default", macaddr: wan_defmac(ctx) };
		if (set_iface_mac(c, macaddr, port)) {
			need_reload = true;
			c.commit("network");
			ctx.ubus.call("network", "reload");
		}
	}
	c.commit("network");

	set_port_mode_noswitch(c, ctx, interface, mode);
	c.commit("board_special");
	c.commit("network");

	if (mode == "wan") {
		let value = c.get("glconfig", "general", interface);
		let vlanid = value ? int(value) : 0;
		if (!set_interface_vlan(c, interface, vlanid, vlanid > 0))
			return { err_code: -1, err_msg: "wan vlan conflicts with dual wan vlan" };
		c.commit("board_special");
		c.commit("network");
	}
	if (need_reload) ctx.ubus.call("network", "reload");
	return null;
}

function set_cron_update_mac(ctx) {
	let path = "/etc/crontabs/root";
	let data = fs.access(path) ? (fs.readfile(path) ?? "") : "";
	for (let line in split(data, "\n")) if (match(line, /gl-update-cable-mac/)) return;
	let nl = (data && substr(data, length(data) - 1) != "\n") ? "\n" : "";
	fs.writefile(path, data + nl + "*/30 * * * * /usr/sbin/gl-update-cable-mac.sh time\n");
	ctx.popen("/etc/init.d/cron restart >/dev/null 2>&1");
}

function is_num(v) { let t = type(v); return t == "int" || t == "double"; }

export default {
	get_status: function(args, ctx) {
		let c = ctx.uci.cursor();
		let res = iface_status(ctx, c, "wan");
		if (res.err_code) return res;
		res.secondwan = iface_status(ctx, c, "secondwan");
		res.usbwan = iface_status(ctx, c, "usbwan");
		return res;
	},

	get_config: function(args, ctx) {
		args = args ?? {};
		let interface = args.usbwan ? "usbwan" : (args.secondwan ? "secondwan" : "wan");
		let c = ctx.uci.cursor();
		let err = check_wan_supported(ctx, c, interface);
		if (err) return err;

		let cfg = c.get_all("network", interface) ?? {};
		let proto = cfg.proto;
		let use_device = c.get("network", "lan", "device");
		let device = c.get("network", interface, use_device ? "device" : "ifname");
		let device_sid = find_device_sid(c, device);
		let mtu = (device_sid) ? c.get("network", device_sid, "mtu")
		                       : c.get("network", interface, "mtu");

		let res = {
			protocol: proto,
			mtu: (mtu != null) ? int(mtu) : null,
			ttl: (cfg.ttl != null) ? int(cfg.ttl) : null,
			ttl_ipv6: (cfg.ttl_ipv6 != null) ? int(cfg.ttl_ipv6) : null
		};
		let vlaninfo = get_interface_vlan(c, interface);
		if (vlaninfo && vlaninfo.tag) res.vlanid = vlaninfo.vlan;

		if (proto == "pppoe") {
			res.username = cfg.username;
			res.password = cfg.password;
		} else if (proto == "static") {
			res.ipv4 = { ip: cfg.ipaddr, gateway: cfg.gateway, netmask: cfg.netmask, dns: cfg.dns };
			let cfg6 = c.get_all("network", interface + "6") ?? {};
			res.ipv6 = { ip: cfg6.ip6addr, gateway: cfg6.ip6gw, dns: cfg6.dns };
		}
		return res;
	},

	set_config: function(args, ctx) {
		args = args ?? {};
		let proto = args.protocol;
		let interface = args.usbwan ? "usbwan" : (args.secondwan ? "secondwan" : "wan");
		let interface6 = interface + "6";

		if (get_port_mode(ctx, interface) != "wan")
			return { err_code: -3, err_msg: "the interface work as lan" };
		if (!proto) return ctx.util.E.INVALID_PARAMS;
		if (proto != "dhcp" && proto != "static" && proto != "pppoe")
			return ctx.util.E.INVALID_PARAMS;

		let c = ctx.uci.cursor();
		let use_device = c.get("network", "lan", "device");

		let vlanid = args.vlanid ?? 0;
		if (!is_num(vlanid)) return ctx.util.E.INVALID_PARAMS;
		if (vlanid < 0 || vlanid > 4094)
			return { err_code: -1, err_msg: "Invalid vlanid, allowed range 0-4094" };
		if (!set_interface_vlan(c, interface, vlanid, vlanid > 0))
			return { err_code: -1, err_msg: "wan vlan conflicts with dual wan vlan" };
		c.set("glconfig", "general", interface, vlanid);
		c.commit("glconfig");
		c.commit("board_special");

		c.delete("network", interface, "ipaddr");
		c.delete("network", interface, "netmask");
		c.delete("network", interface, "gateway");
		c.delete("network", interface, "dns");
		c.delete("network", interface, "peerdns");
		c.delete("network", interface, "disabled");
		c.set("network", interface, "proto", proto);

		let mtu = args.mtu ?? 0;
		if (!is_num(mtu)) return ctx.util.E.INVALID_PARAMS;
		if (mtu != 0 && (mtu < 576 || mtu > 9200 || mtu != int(mtu)))
			return { err_code: -1, err_msg: "Invalid mtu value, allowed range 576-9200, must be integer" };

		let device = c.get("network", interface, use_device ? "device" : "ifname");
		let device_sid = find_device_sid(c, device);
		if (mtu > 0) {
			if (device_sid) c.set("network", device_sid, "mtu", mtu);
			else c.set("network", interface, "mtu", mtu);
		} else {
			if (device_sid) c.delete("network", device_sid, "mtu");
			else c.delete("network", interface, "mtu");
		}

		let ttl = args.ttl ?? 0;
		let ttl_ipv6 = args.ttl_ipv6 ?? 0;
		if (!is_num(ttl)) return ctx.util.E.INVALID_PARAMS;
		if (ttl != 0 && (ttl < 1 || ttl > 255 || ttl != int(ttl)))
			return { err_code: -1, err_msg: "Invalid ttl value, allowed range 1-255, must be integer" };
		if (!is_num(ttl_ipv6)) return ctx.util.E.INVALID_PARAMS;
		if (ttl_ipv6 != 0 && (ttl_ipv6 < 1 || ttl_ipv6 > 255 || ttl_ipv6 != int(ttl_ipv6)))
			return { err_code: -1, err_msg: "Invalid ttl_ipv6 value, allowed range 1-255, must be integer" };

		c.set("network", interface, "ttl", ttl > 0 ? ttl : "");
		c.set("network", interface, "ttl_ipv6", ttl_ipv6 > 0 ? ttl_ipv6 : "");

		if (fs.access("/sbin/fw4")) {
			let nftdir = "/usr/share/nftables.d/chain-pre/mangle_postrouting";
			let p4 = `${nftdir}/01-ethernet-set-ttl-${interface}.nft`;
			if (ttl > 0) {
				ctx.popen(`mkdir -p ${nftdir}`);
				let data = (proto == "pppoe")
					? sprintf("oifname pppoe-%s ip ttl set %d\n", interface, int(ttl))
					: sprintf("oifname %s ip ttl set %d\n", device, int(ttl));
				fs.writefile(p4, data);
			} else {
				ctx.popen(`rm -f ${p4}`);
			}
			let p6 = `${nftdir}/01-ethernet-hoplimit-set-ttl_ipv6-${interface}.nft`;
			if (ttl_ipv6 > 0) {
				ctx.popen(`mkdir -p ${nftdir}`);
				let data = (proto == "pppoe")
					? sprintf("oifname pppoe-%s ip6 hoplimit != 255 ip6 hoplimit set %d\n", interface, int(ttl_ipv6))
					: sprintf("oifname %s ip6 hoplimit != 255 ip6 hoplimit set %d\n", device, int(ttl_ipv6));
				fs.writefile(p6, data);
			} else {
				ctx.popen(`rm -f ${p6}`);
			}
		} else {
			c.set("firewall", "ethernet_ttl", "include");
			c.set("firewall", "ethernet_ttl", "type", "script");
			c.set("firewall", "ethernet_ttl", "reload", "1");
			c.set("firewall", "ethernet_ttl", "path", "/etc/firewall.ethernet_ttl");
		}

		c.set("network", interface6, "proto", "dhcpv6");
		c.delete("network", interface6, "ip6addr");
		c.delete("network", interface6, "ip6gw");
		c.delete("network", interface6, "dns");

		if (proto == "static") {
			let ipv4 = args.ipv4 ?? {};
			if (!valid_ip_addr(ipv4.ip)) return ctx.util.E.INVALID_PARAMS;
			if (!valid_ip_addr(ipv4.netmask)) return ctx.util.E.INVALID_PARAMS;
			if (!valid_ip_addr(ipv4.gateway)) return ctx.util.E.INVALID_PARAMS;
			let dns = ipv4.dns;
			if (!dns) return { err_code: -1, err_msg: "dns missing" };
			if (type(dns) != "array") return ctx.util.E.INVALID_PARAMS;
			if (length(dns) < 1) return { err_code: -1, err_msg: "dns missing" };
			for (let d in dns) if (!valid_ip_addr(d)) return ctx.util.E.INVALID_PARAMS;
			// DEGRADE: hardware.platform_vendor_port_config_setting -> always plain uci.
			c.set("network", interface, "ipaddr", ipv4.ip);
			c.set("network", interface, "netmask", ipv4.netmask);
			c.set("network", interface, "gateway", ipv4.gateway);
			c.set("network", interface, "dns", dns);
			c.set("network", interface, "peerdns", 0);
			let ipv6 = args.ipv6;
			if (ipv6) {
				c.set("network", interface6, "proto", "static");
				c.set("network", interface6, "ip6addr", ipv6.ip ?? "");
				c.set("network", interface6, "ip6gw", ipv6.gateway ?? "");
				c.set("network", interface6, "dns", ipv6.dns ?? "");
			}
		} else if (proto == "pppoe") {
			let username = args.username, password = args.password;
			if (username && length(username) > 256) return ctx.util.E.INVALID_PARAMS;
			if (password && length(password) > 256) return ctx.util.E.INVALID_PARAMS;
			if (fs.access("/tmp/pppoe_fail_count")) ctx.popen("rm -f /tmp/pppoe_fail_count");
			c.set("network", interface, "username", username ?? "");
			c.set("network", interface, "password", password ?? "");
		}
		// DEGRADE: dhcp+wan hardware.platform_vendor_port_config_setting dropped.

		set_accept_ra_for_wan(ctx, proto, device);

		// add interface to wan firewall zone (idempotent)
		c.foreach("firewall", "zone", function(s) {
			if (s.name != "wan") return;
			let nets = s.network ?? [];
			if (type(nets) == "string") nets = string_split(nets, " ");
			if (type(nets) != "array") nets = [];
			for (let n in nets) if (n == interface) return;
			push(nets, interface);
			c.set("firewall", s[".name"], "network", nets);
		});

		c.commit("firewall");
		c.commit("network");
		ctx.popen("sync");
		check_lpm_wakeup(ctx);
		ctx.popen("/etc/init.d/firewall reload >/dev/null 2>&1");
		ctx.ubus.call("network", "reload");
		// sft1200: bounce the interface so the new proto/vlan takes effect.
		ctx.popen(`(sleep 2; ifdown ${interface}; ifup ${interface}) >/dev/null 2>&1 &`);
		return {};
	},

	get_ports_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let ports = [];
		let wport = wan_port(ctx);
		if (wport) {
			let p = {
				name: "wan",
				silk: "WAN",
				mode: get_port_mode(ctx, "wan"),
				default_mode: is_lan_port(c, wport) ? "lan" : "wan",
				macaddr: { defmacaddr: wan_defmac(ctx) }
			};
			get_port_mac_config(ctx, p, wport);
			push(ports, p);
		}
		// DEGRADE: secondwan / usbwan absent on this board.
		for (let info in LAN_PORTS)
			push(ports, { name: info.name, mode: "lan", silk: info.silk });
		return { ports: ports };
	},

	get_ports_status: function(args, ctx) {
		let ports = [];
		let wport = wan_port(ctx);
		if (wport) push(ports, { name: "wan", port: wport });
		for (let info in LAN_PORTS)
			push(ports, { name: info.name, port: info.device });

		let out = [];
		for (let port in ports) {
			let speed = null, duplex = null;
			// DEGRADE: swconfig removed -> netifd device status on the DSA slave netdev.
			let s = ctx.ubus.call("network.device", "status", { name: port.port }) ?? {};
			if (s.carrier && s.speed) {
				let m = match(s.speed, /([0-9]+)([a-zA-Z])/);
				if (m) { speed = int(m[1]); duplex = (m[2] == "F") ? "full" : "half"; }
			}
			push(out, { name: port.name, speed: speed, duplex: duplex });
		}
		return { ports: out };
	},

	set_port_config: function(args, ctx) {
		args = args ?? {};
		let name = args.name;
		// TAP-S2S guard (kept: same uci names on mainline if ovpn present)
		let c0 = ctx.uci.cursor();
		let config_name = c0.get("network", "ovpnclient", "config");
		let client_mode = config_name ? c0.get("ovpnclient", config_name, "mode") : "";
		if (client_mode == "tap-s2s") {
			let disabled = c0.get("network", "ovpnclient", "disabled") ?? "1";
			if (disabled == "0")
				return { err_code: 20014007, err_msg: "function does not work because TAP-S2S Mode is enabled" };
		}

		let port = iface_port(ctx, name);
		if (!port) return { err_code: -1, err_msg: "unsupport" };

		let mode = args.mode;
		let need_reload = false, need_restart = false, route_cleanup_device = null;
		if (mode) {
			if (mode != "wan" && mode != "lan")
				return { err_code: -2, err_msg: "unsupported work mode" };
			let oldmode = get_port_mode(ctx, name);
			if (mode != oldmode) {
				let c = ctx.uci.cursor();
				if (oldmode == "wan" && mode == "lan")
					route_cleanup_device = get_interface_dev(c, name) ?? port;
				let errs = set_port_mode(ctx, name, mode, port, route_cleanup_device);
				if (errs) return errs;
				need_restart = true;
			}
		}

		if (name != "usbwan") {
			let macaddr = args.macaddr;
			if (macaddr && macaddr.mode != "default" && macaddr.mode != "clone" && macaddr.mode != "random")
				return ctx.util.E.INVALID_PARAMS;
			let c = ctx.uci.cursor();
			if (name == "wan" || name == "secondwan") {
				if (macaddr && (macaddr.mode == "random" || macaddr.mode == "clone"))
					c.set("network", name, "hostname", "*");
				else
					c.delete("network", name, "hostname");
				c.commit("network");
			}
			if (macaddr) {
				if (set_iface_mac(c, macaddr, port)) need_reload = true;
				c.commit("network");
			}
			set_cron_update_mac(ctx);
		}

		ctx.popen("sync");
		check_lpm_wakeup(ctx);
		if (need_restart) {
			ctx.ubus.call("network", "reload");
			ctx.popen("/etc/init.d/gl_ipv6 reload >/dev/null 2>&1");
			clear_ipv6_routes_by_device(ctx, route_cleanup_device);
		} else if (need_reload) {
			ctx.ubus.call("network", "reload");
		}
		return {};
	},

	wan_proto_detect: function(args, ctx) {
		let c = ctx.uci.cursor();
		let internet = iface_online(ctx, "wan");
		let wport = readfile("/proc/gl-hw-info/wan") ?? wan_port(ctx);
		let suggest_wan_proto;
		if (wport) {
			let cur = c.get("network", "wan", "proto") ?? "";
			if (internet == true && (cur == "pppoe" || cur == "dhcp"))
				return { internet: internet, suggest_proto: cur };
			let success_dhcp = false, success_pppoe = false;
			suggest_wan_proto = "no";
			for (let i = 0; i < 4; i++) {
				// DEGRADE: shell out to pppoe-discovery/dhcpdiscover if present.
				if (!success_pppoe && ctx.popen(`pppoe-discovery -I ${wport} -t 1 -a 1 >/dev/null 2>&1; echo $?`) == "0") {
					suggest_wan_proto = "pppoe";
					break;
				}
				if (!success_dhcp && ctx.popen(`dhcpdiscover -i ${wport} -t 1 >/dev/null 2>&1; echo $?`) == "0") {
					suggest_wan_proto = "dhcp";
					success_dhcp = true;
				}
			}
		}
		return { internet: internet, suggest_proto: suggest_wan_proto };
	},

	init_status: function(args, ctx) {
		let c = ctx.uci.cursor();
		let wan = false, lan = false;
		let internet = iface_online(ctx, "wan");
		let proto = c.get("network", "wan", "proto") ?? "";
		let wport = readfile("/proc/gl-hw-info/wan") ?? wan_port(ctx);
		if (wport && port_carrier(ctx, wport)) wan = true;

		// DEGRADE: swconfig removed -> link up on any lan netdev == lan present.
		for (let info in LAN_PORTS) {
			let s = ctx.ubus.call("network.device", "status", { name: info.device }) ?? {};
			if (s.carrier && s.speed) {
				let m = match(s.speed, /([0-9]+)([a-zA-Z])/);
				if (m) { lan = true; break; }
			}
		}
		return { wan: wan, lan: lan, proto: proto, internet: internet };
	}
};

// ---- IPv4 validator (kept at bottom: referenced by set_config) --------------------
function valid_ip_addr(ip) {
	if (!ip) return false;
	let m = match(ip, /^([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)$/);
	if (!m) return false;
	for (let i = 1; i <= 4; i++) if (int(m[i]) > 255) return false;
	return true;
}
