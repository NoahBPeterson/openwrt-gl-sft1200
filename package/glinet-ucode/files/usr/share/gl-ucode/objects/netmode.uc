// gl-ucode object: netmode  (port of decompiled-spec/rpc/netmode.lua)
// Working-mode (router / ap / relay / wds) switch, rebuilt on stock OpenWrt
// primitives for the SFT1200 (Siflower SF1688). wan port = eth0.2, lan = eth0.1,
// bridged as br-lan (per system.uc).
//
// STORE: the current mode is kept in the SAME uci key the original used,
//   `glconfig.general.mode`, so the other ported objects that gate on it
//   (e.g. tailscale) interoperate. The section is created on demand; nothing
//   else in `glconfig` is assumed to exist.
//
// DEGRADE / dropped GL sources (no mainline equivalent on this device):
//   - passthrough / modem / cellular: DROPPED entirely (gl_cellular_manager,
//     kmwan multi-wan, passthrough uci + ubus, set_modem, secondwan/usbethernet
//     port juggling). No modem on the SFT1200, so `passthrough` is not a mode.
//   - hardware.set_wan_ports_to_vlan / platform_netmode_set_back_router /
//     platform_switch_restart: REPLACED by direct br-lan bridge-membership edits
//     (add/remove eth0.2) + `ubus network reload`.
//   - GL repeater daemon (ubus `repeater` connect/status/save_config, relayd
//     choreography): the STA join + connection-verification loop is DEGRADED to
//     writing a stock mac80211 STA wifi-iface + wwan interface (+ relay stabridge
//     for relay mode) and reloading. There is no daemon here to poll join state,
//     so relay/wds apply the config optimistically (see notes at bottom).
//   - Dropped subsystems: gl_dpi, sqm, tor, gl_screen, gl-vlan-lifecycle,
//     gl-black_white_list, gl_ipv6, edgerouter, gl-cloud, reset_network_utils.
//   - kick_all_stations is kept ONLY for mac80211 (hostapd del_client broadcast);
//     all mtk/qca/iwpriv/mwctl branches dropped.
'use strict';

const fs = require('fs');

const WAN_DEV_DEFAULT = "eth0.2";      // SFT1200 wan port
const LAN_IP_DEFAULT  = "192.168.8.1"; // GL default LAN IP (LAN-recovery safety)

// ---- uci helpers --------------------------------------------------------

// Name of the anonymous `device` section whose .name == "br-lan".
function br_dev_name(c) {
	let name = null;
	c.foreach("network", "device", function(s) {
		if (s.name == "br-lan" && name == null) name = s[".name"];
	});
	return name;
}

// Ports list of a `device` section as an array (uci may hand back a bare string).
function get_ports(c, secname) {
	let p = c.get("network", secname, "ports");
	if (p == null) return [];
	if (type(p) == "string") return [p];
	return p;
}

// Ensure `wan` is a member of the br-lan bridge (AP / WDS / relay).
function bridge_add_wan(c, wan) {
	let sec = br_dev_name(c);
	if (!sec || !wan) return;
	let np = filter(get_ports(c, sec), function(x) { return x != wan; });
	push(np, wan);
	c.set("network", sec, "ports", np);
}

// Remove `wan` from the br-lan bridge (router).
function bridge_del_wan(c, wan) {
	let sec = br_dev_name(c);
	if (!sec || !wan) return;
	let np = filter(get_ports(c, sec), function(x) { return x != wan; });
	if (length(np) == 0) c.delete("network", sec, "ports");
	else c.set("network", sec, "ports", np);
}

function first_radio(c) {
	let r = null;
	c.foreach("wireless", "wifi-device", function(s) { if (r == null) r = s[".name"]; });
	return r;
}

// Disable guest/iot AP wifi-ifaces + LAN dnsmasq/dhcp when bridging into LAN.
// (mac80211 only; ports the harmless subset of set_bridge_wifi.)
function set_bridge_wifi(c) {
	c.foreach("wireless", "wifi-iface", function(s) {
		if (s.mode == "ap" && (s.guest == "1" || s.iot == "1"))
			c.set("wireless", s[".name"], "disabled", 1);
	});
	c.set("dhcp", "lan", "ignore", 1);
	c.set("dhcp", "@dnsmasq[0]", "disabled", "1");
}

// uci has no rename via ctx cursor -> emulate get/set/delete.
function rename_opt(c, cfg, sec, from_opt, to_opt) {
	let v = c.get(cfg, sec, from_opt);
	if (v != null) { c.set(cfg, sec, to_opt, v); c.delete(cfg, sec, from_opt); }
}

// ---- runtime helpers (mac80211) -----------------------------------------

// Kick all associated stations off every up AP (hostapd broadcast del_client).
function kick_all_stations(ctx) {
	let radios = ctx.ubus.call("network.wireless", "status") ?? {};
	for (let name in radios) {
		let radio = radios[name];
		if (!radio || !radio.up || !radio.interfaces) continue;
		for (let iface in radio.interfaces) {
			let cfg = iface.config ?? {};
			if (cfg.mode == "ap" && cfg.ifname)
				ctx.ubus.call(`hostapd.${cfg.ifname}`, "del_client",
					{ addr: "ff:ff:ff:ff:ff:ff" });
		}
	}
}

// Write a stock mac80211 STA wifi-iface joining `params.ssid` on the given
// network. DEGRADE: replaces GL's `repeater connect` (no join-state daemon here).
function setup_sta(c, params, network, wds) {
	let radio = params.device ?? first_radio(c);
	c.set("wireless", "sta", "wifi-iface");
	if (radio) c.set("wireless", "sta", "device", radio);
	c.set("wireless", "sta", "mode", "sta");
	c.set("wireless", "sta", "network", network);
	c.set("wireless", "sta", "ssid", params.ssid);
	if (params.bssid) c.set("wireless", "sta", "bssid", params.bssid);
	if (params.key && params.key != "") {
		c.set("wireless", "sta", "encryption", params.encryption ?? "psk2");
		c.set("wireless", "sta", "key", params.key);
	} else {
		c.set("wireless", "sta", "encryption", "none");
		c.delete("wireless", "sta", "key");
	}
	if (wds) c.set("wireless", "sta", "wds", "1");
	else c.delete("wireless", "sta", "wds");
	c.set("wireless", "sta", "disabled", "0");
}

// ---- apply choreography (network-affecting; set_mode only) ---------------

function apply(ctx, mode) {
	ctx.ubus.call("network", "reload");
	ctx.popen("/etc/init.d/dnsmasq restart >/dev/null 2>&1");
	ctx.popen("/etc/init.d/firewall reload >/dev/null 2>&1");
	if (mode == "relay")
		ctx.popen("/etc/init.d/relayd restart >/dev/null 2>&1");
	if (mode != "router") {
		kick_all_stations(ctx);
	}
}

// -------------------------------------------------------------------------

export default {
	get_mode: function(args, ctx) {
		let c = ctx.uci.cursor();
		// passthrough branch dropped (no modem): always the simple shape.
		return { mode: c.get("glconfig", "general", "mode") ?? "router" };
	},

	set_mode: function(args, ctx) {
		let c = ctx.uci.cursor();
		let mode = args.mode;
		if (type(mode) != "string")
			return ctx.util.E.INVALID_PARAMS;
		if (mode != "router" && mode != "ap" && mode != "relay" && mode != "wds")
			return ctx.util.E.INVALID_PARAMS;

		let wan = c.get("network", "wan", "device") ?? WAN_DEV_DEFAULT;

		// GL firewall "Block_Nonlocal" hardening toggle (only if present).
		if (c.get("firewall", "Block_Nonlocal") != null)
			c.set("firewall", "Block_Nonlocal", "enabled", (mode == "router") ? 1 : 0);

		if (mode == "ap") {
			// Bridge wan into LAN, become a dumb AP: no DHCP, no WAN, LAN via DHCP.
			set_bridge_wifi(c);
			c.set("network", "wan", "disabled", 1);
			c.delete("network", "stabridge");
			bridge_add_wan(c, wan);
			c.set("network", "lan", "proto", "dhcp");
			rename_opt(c, "network", "lan", "ipaddr", "ipaddr_old");
			c.set("glconfig", "general", "mode", "ap");

		} else if (mode == "router") {
			// Restore stock: wan back off the bridge, LAN static + serving DHCP.
			bridge_del_wan(c, wan);
			c.set("network", "lan", "proto", "static");
			rename_opt(c, "network", "lan", "ipaddr_old", "ipaddr");
			// LAN-recovery safety: never leave a static LAN with no address.
			if (c.get("network", "lan", "ipaddr") == null)
				c.set("network", "lan", "ipaddr", LAN_IP_DEFAULT);
			c.set("network", "wan", "disabled", 0);
			c.delete("network", "stabridge");
			c.delete("wireless", "sta");
			c.set("dhcp", "lan", "ignore", 0);
			c.set("dhcp", "@dnsmasq[0]", "disabled", "0");
			c.set("glconfig", "general", "mode", "router");

		} else if (mode == "relay") {
			// Repeater (relayd): STA on wwan, relay bridge stabridge over lan+wwan.
			if (type(args.ssid) != "string")
				return ctx.util.E.INVALID_PARAMS;
			setup_sta(c, args, "wwan", false);
			c.set("network", "wwan", "interface");
			c.set("network", "wwan", "proto", "dhcp");
			c.set("network", "stabridge", "interface");
			c.set("network", "stabridge", "proto", "relay");
			c.set("network", "stabridge", "network", "lan wwan");
			c.set("glconfig", "general", "mode", "relay");

		} else if (mode == "wds") {
			// WDS: 4addr STA bridged straight into LAN; behave like AP otherwise.
			if (type(args.ssid) != "string")
				return ctx.util.E.INVALID_PARAMS;
			setup_sta(c, args, "lan", true);
			set_bridge_wifi(c);
			bridge_add_wan(c, wan);
			c.set("network", "wan", "disabled", "1");
			c.set("network", "lan", "proto", "dhcp");
			rename_opt(c, "network", "lan", "ipaddr", "ipaddr_old");
			c.delete("network", "stabridge");
			c.set("glconfig", "general", "mode", "wds");
		}

		// firewall: allow wan->lan forwarding only in relay mode.
		if (mode == "relay") {
			c.set("firewall", "forwarding_wan2lan", "forwarding");
			c.set("firewall", "forwarding_wan2lan", "src", "wan");
			c.set("firewall", "forwarding_wan2lan", "dest", "lan");
		} else {
			c.delete("firewall", "forwarding_wan2lan");
		}

		// DEGRADE: GLACL rule disable loop kept (guarded, harmless if none exist).
		if (mode != "router") {
			c.foreach("firewall", "rule", function(s) {
				if (s.name && substr(s.name, 0, 5) == "GLACL" && s.enabled != "0")
					c.set("firewall", s[".name"], "enabled", "0");
			});
		}

		// ensure the glconfig.general store section exists, then persist.
		if (!c.get("glconfig", "general")) c.set("glconfig", "general", "general");
		c.set("glconfig", "general", "mode", mode);

		c.commit("network");
		c.commit("wireless");
		c.commit("dhcp");
		c.commit("firewall");
		c.commit("glconfig");
		system("sync 2>/dev/null");

		apply(ctx, mode);

		if (mode == "router")
			return { lanip: c.get("network", "lan", "ipaddr") ?? LAN_IP_DEFAULT };
		return {};
	}
};
