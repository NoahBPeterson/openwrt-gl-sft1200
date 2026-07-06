// gl-ucode object: network  (port of decompiled-spec/rpc/network.lua)
// WAN/network status + config. GL-only sources are degraded to mainline:
//   - lfactory / hardware.platform_get_*wan_status  -> sysfs /sys/class/net/<dev>/carrier
//   - glipv6 / glconfig / gl_ddns / kmwan / modem RPC -> absent-tolerant defaults
//   - MediaTek/QCA NAT accel (mtkhnat / ecm / ipacm) -> DROPPED; only the mainline
//     firewall flow-offloading path is kept (Siflower SF1688 is mac80211 + netfilter).
'use strict';

const fs = require('fs');

function readfile(p) { let d = fs.readfile(p); return d ? trim(d) : null; }

// DROP: MTK/QCA branches; this only detects a mainline-visible HW offload path.
function has_sft_hnat() {
	return fs.access("/sys/kernel/debug/hnat_debug")
	    || fs.access("/sys/bus/platform/drivers/dpns")
	    || fs.access("/sys/bus/platform/drivers/mt7621-pci-phy");
}

function get_default_route_gateway(status) {
	let routes = (status && status.route) ? status.route : [];
	for (let route in routes) {
		if (route.target == "0.0.0.0" && int(route.mask) == 0) {
			let g = route.nexthop;
			if (type(g) == "string" && g != "" && g != "0.0.0.0") return g;
		}
	}
	return null;
}

// DEGRADE: original walked kmwan members + modem RPC; here we scan the mainline
// uplink interfaces for the one that carries a default route.
function get_available_wan_info(ctx) {
	for (let ifn in ["wan", "internet"]) {
		let s = ctx.ubus.call(`network.interface.${ifn}`, "status");
		if (!s) continue;
		let ip = (s["ipv4-address"] && s["ipv4-address"][0]) ? s["ipv4-address"][0].address : "";
		let gw = get_default_route_gateway(s);
		if (type(gw) == "string" && gw != "") return { ip: (type(ip) == "string" ? ip : ""), gateway: gw };
	}
	return null;
}

// DEGRADE: kmwan/modem absent -> just take the first non-empty v4 + v6 uplink address.
function get_available_wan_ip(ctx) {
	let wan_ips = [];
	let v4, v6;
	for (let ifn in ["wan", "internet"]) {
		let s = ctx.ubus.call(`network.interface.${ifn}`, "status");
		if (s && s["ipv4-address"] && s["ipv4-address"][0]) {
			let a = s["ipv4-address"][0].address;
			if (a && a != "") { v4 = a; break; }
		}
	}
	for (let ifn in ["wan6", "wan"]) {
		let s = ctx.ubus.call(`network.interface.${ifn}`, "status");
		if (s && s["ipv6-address"] && s["ipv6-address"][0]) {
			let a = s["ipv6-address"][0].address;
			if (a && a != "") { v6 = a; break; }
		}
	}
	if (v4) push(wan_ips, v4);
	if (v6) push(wan_ips, v6);
	return wan_ips;
}

function get_public_address_list(ctx) {
	let urls = ["ip.me", "ip.gs", "ifconfig.me/ip"];
	let ips = [];
	for (let v in urls) {
		let dat = ctx.popen(`curl -4 -f -s -m 1 ${v} 2>/dev/null`);
		dat = (type(dat) == "string") ? trim(dat) : "";
		if (match(dat, /[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+/)) { push(ips, dat); break; }
	}
	for (let v in urls) {
		let dat = ctx.popen(`curl -6 -f -s -m 1 ${v} 2>/dev/null`);
		dat = (type(dat) == "string") ? trim(dat) : "";
		if (match(dat, /^[0-9a-fA-F:]+$/) && index(dat, ":") >= 0) { push(ips, dat); break; }
	}
	return ips;
}

// DEGRADE: gl_ddns config + /proc/gl-hw-info/device_ddns absent -> empty list.
function get_ddns_list(ctx) {
	let domains = [];
	let c = ctx.uci.cursor();
	let d = c.get("gl_ddns", "glddns", "domain");
	if (d) {
		push(domains, d);
	} else {
		let dev = readfile("/proc/gl-hw-info/device_ddns");
		if (dev) push(domains, `${dev}.glddns.com`);
	}
	return domains;
}

export default {
	get_status: function(args, ctx) {
		let out = { interfaces: [] };
		for (let ifn in ["wan", "lan", "wwan"]) {
			let s = ctx.ubus.call(`network.interface.${ifn}`, "status");
			if (!s) continue;
			let addr = (s["ipv4-address"] && s["ipv4-address"][0]) ? s["ipv4-address"][0] : {};
			push(out.interfaces, {
				interface: ifn,
				up: s.up ? true : false,
				proto: s.proto ?? "",
				device: s.l3_device ?? s.device ?? "",
				ipaddr: addr.address ?? "",
				mask: addr.mask ?? null
			});
		}
		return out;
	},

	get_arp_list: function(args, ctx) {
		let list = [];
		let out = ctx.popen("ip neigh show 2>/dev/null");
		if (type(out) != "string") out = "";   // narrow string|null -> string
		for (let line in split(out, "\n")) {
			let m = match(line, /^([0-9.]+) dev (\S+) lladdr ([0-9a-fA-F:]{17}) (\w+)/);
			if (m) push(list, { ip: m[1], device: m[2], mac: lc(m[3]), state: m[4] });
		}
		return { arp: list };
	},

	get_network_info: function(args, ctx) {
		let c = ctx.uci.cursor();
		return {
			lan_ip: c.get("network", "lan", "ipaddr") ?? "",
			lan_netmask: c.get("network", "lan", "netmask") ?? "",
			wan_proto: c.get("network", "wan", "proto") ?? ""
		};
	},

	// DEGRADE: glipv6 config absent on mainline -> only bail when explicitly disabled.
	get_ipv6_neigh_list: function(args, ctx) {
		let c = ctx.uci.cursor();
		if (c.get("glipv6", "globals", "enabled") == "0") return {};
		let entries = [], seen = {};
		let out = ctx.popen("ip -6 neighbour 2>/dev/null");
		if (type(out) != "string") out = "";
		for (let line in split(out, "\n")) {
			let m = match(line, /^(\S+) +dev +(\S+) +lladdr +(\S+)/);
			if (m) {
				let macaddr = uc(m[3]);
				if (macaddr != "00:00:00:00:00:00" && !seen[macaddr]) {
					push(entries, { ip: m[1], mac: macaddr, device: m[2] });
					seen[macaddr] = true;
				}
			}
		}
		return { entries: entries };
	},

	// DEGRADE: hardware.platform_get_*wan_status (GL C helper) absent -> read sysfs carrier.
	// secondwan/usbwan (multi-WAN) + macclone (glconfig) not present on this image -> defaults.
	check_wan_cable: function(args, ctx) {
		let s = ctx.ubus.call("network.interface.wan", "status") ?? {};
		let dev = s.l3_device ?? s.device ?? "eth0.2";
		let carrier = readfile(`/sys/class/net/${dev}/carrier`);
		let cable = (carrier == "1");
		return {
			cable_enabled: cable,
			macclone_enabled: false,
			cable_inserted: cable
		};
	},

	// DROP: mtkhnat / ecm(qca) / ipacm branches (other GL SoCs). Keep only the mainline
	// firewall flow-offloading path. qos/dpi feature-conflict probes degrade to false.
	get_netnat_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let actype = null;

		let qos_enabled = false;
		c.foreach("qos", "queue", function(s) { qos_enabled = true; });

		let dpi_enabled = false;
		if (fs.access("/usr/sbin/netifyd")) {
			let flow_statistics_enabled = c.get("gl_dpi_flow_statistics", "global", "enable") == "1";
			let prio_enabled = c.get("gl_dpi_qos", "qos", "prio_enable") == "1";
			let content_protection_enabled = c.get("gl_dpi_content_protection", "content_protection", "enabled") == "1";
			let sqm_enabled = c.get("sqm", "@queue[0]", "enabled") == "1";
			if (flow_statistics_enabled || prio_enabled || content_protection_enabled || sqm_enabled)
				dpi_enabled = true;
		}

		let list = [2];              // 2 = software flow-offloading
		let has_hw = false;
		if (fs.access("/sys/kernel/debug/ppe0")) { push(list, 1); has_hw = true; }
		if (has_sft_hnat() && !has_hw) { push(list, 1); has_hw = true; }

		let flow_offloading = c.get("firewall", "@defaults[0]", "flow_offloading") == "1";
		let flow_offloading_hw = has_hw && c.get("firewall", "@defaults[0]", "flow_offloading_hw") == "1";
		if (flow_offloading_hw) actype = 1;
		else if (flow_offloading) actype = 2;

		let enable = actype ? true : false;
		if (!enable) {
			flow_offloading = c.get("firewall", "@defaults[0]", "flow_offloading_old") == "1";
			flow_offloading_hw = has_hw && c.get("firewall", "@defaults[0]", "flow_offloading_hw_old") == "1";
			if (flow_offloading_hw) actype = 1;
			else if (flow_offloading) actype = 2;
			else if (has_hw) actype = 1;
			else actype = 2;
		}

		return {
			enable: enable,
			actype: actype,
			list: list,
			wifi_reload: false,
			conflict_features: [],
			qos_enabled: qos_enabled,
			dpi_enabled: dpi_enabled
		};
	},

	// DROP: mtkhnat/ecm/ipacm writes + set_packet_steer. Mainline firewall path only.
	set_netnat_config: function(args, ctx) {
		args ??= {};
		let c = ctx.uci.cursor();
		let enable = args.enable;
		let actype = args.actype;

		let parental_control_enabled = false;
		if (fs.access("/etc/config/parental_control_v2"))
			parental_control_enabled = c.get("parental_control_v2", "global", "enable") == "1";
		else
			parental_control_enabled = c.get("parental_control", "global", "enable") == "1";

		let qos_enabled = false;
		c.foreach("qos", "queue", function(s) { qos_enabled = true; });

		let dpi_enabled = false;
		if (fs.access("/usr/sbin/netifyd")) {
			let flow_statistics_enabled = c.get("gl_dpi_flow_statistics", "global", "enable") == "1";
			let prio_enabled = c.get("gl_dpi_qos", "qos", "prio_enable") == "1";
			let content_protection_enabled = c.get("gl_dpi_content_protection", "content_protection", "enabled") == "1";
			let sqm_enabled = c.get("sqm", "@queue[0]", "enabled") == "1";
			if (flow_statistics_enabled || prio_enabled || content_protection_enabled || sqm_enabled)
				dpi_enabled = true;
		}

		if (enable && (parental_control_enabled || qos_enabled || dpi_enabled)) {
			return {
				err_code: -1,
				err_msg: "Please disable Parental Control/QoS/DPI before changing NAT acceleration settings."
			};
		}

		if (enable) {
			c.set("firewall", "@defaults[0]", "flow_offloading", "1");
			c.set("firewall", "@defaults[0]", "flow_offloading_hw", (actype == 1) ? "1" : "0");
		} else {
			let fo = c.get("firewall", "@defaults[0]", "flow_offloading") ?? "0";
			let fhw = c.get("firewall", "@defaults[0]", "flow_offloading_hw") ?? "0";
			c.set("firewall", "@defaults[0]", "flow_offloading_old", fo);
			c.set("firewall", "@defaults[0]", "flow_offloading_hw_old", fhw);
			c.set("firewall", "@defaults[0]", "flow_offloading", "0");
			c.set("firewall", "@defaults[0]", "flow_offloading_hw", "0");
		}
		c.commit("firewall");
		ctx.popen("/etc/init.d/firewall restart >/dev/null 2>&1");
		return {};
	},

	// DEGRADE: fullconenat/sip_alg packages usually absent -> defaults to 0.
	get_advance_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		return {
			nat_enable: int(c.get("fullconenat", "global", "enable") ?? "0"),
			sip_enable: int(c.get("sip_alg", "global", "enable") ?? "0")
		};
	},

	// DEGRADE: writes/restarts are no-ops when the backing packages are absent.
	set_advance_config: function(args, ctx) {
		args ??= {};
		let c = ctx.uci.cursor();
		let nat_enable = args.nat_enable ?? 0;
		let sip_enable = args.sip_enable ?? 0;
		c.set("fullconenat", "global", "enable", `${nat_enable}`);
		c.set("sip_alg", "global", "enable", `${sip_enable}`);
		c.commit("fullconenat");
		c.commit("sip_alg");
		ctx.popen("/etc/init.d/fullconenat restart >/dev/null 2>&1");
		ctx.popen("/etc/init.d/sip_alg restart >/dev/null 2>&1");
		return {};
	},

	get_available_gateway_ip: function(args, ctx) {
		let info = get_available_wan_info(ctx) ?? {};
		return {
			gateway_ip: info.gateway ?? "",
			internal_ip: info.ip ?? ""
		};
	},

	get_available_address_list: function(args, ctx) {
		return {
			pub_ip: get_public_address_list(ctx),
			domain: get_ddns_list(ctx),
			wan_ip: get_available_wan_ip(ctx)
		};
	}
};
