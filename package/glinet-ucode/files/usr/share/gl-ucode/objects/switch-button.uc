// gl-ucode object: switch-button  (port of decompiled-spec/rpc/switch-button.lua)
// Binds the physical mode switch to a function. The GL infra this rests on is ABSENT on
// mainline: the /etc/gl-switch.d/*.sh handler scripts, the gl_switch_button_check daemon,
// and the /proc/gl-hw-info/switch-button GPIO map. The SFT1200 (SF1688) has no usable mode
// switch (system.uc reports switch_button:"0"). DEGRADE: graceful stub that still round-trips
// config. get_funcs -> {funcs:[]} (no handler scripts to enumerate); check_sync_status is a
// no-op {sync_status:1} (btn GPIO + web-state cross-reads dropped); set_config validates types
// and stores to our own `switch-button` uci but performs NO enforcement (no daemon to kick).
// DROPPED from the Lua: /etc/gl-switch.d scan, wireless main/guest-wifi enumeration, the
// route_policy(vpn)/tor/adguardhome/repeater/cellular/led cross-reads, GPIO reads of
// /sys/kernel/debug/gpio, and the gl_switch_button_check restart.
'use strict';

const fs = require('fs');

// Our own store. Mainline has no /etc/config/switch-button; read the section the original used
// (@main[0]) but fall back to a named 'main' section, since that is what our set_config writes.
function read_store(ctx, opt) {
	let c = ctx.uci.cursor();
	return c.get("switch-button", "@main[0]", opt) ?? c.get("switch-button", "main", opt);
}

// Physical switch position. The hotplug handler writes it to /tmp/gl-switch-pos on
// every flip; read that instead of polling /sys/kernel/debug/gpio (hammering that
// debugfs crashes the out-of-tree siflower gpio driver). One safe fallback read if
// the file is missing (no flip since boot).
function switch_on(ctx) {
	let v = fs.readfile("/tmp/gl-switch-pos");
	if (v != null) return trim(v) == "1";
	let g = ctx.popen("grep -m1 mode /sys/kernel/debug/gpio 2>/dev/null");
	return (g && match(g, / in  lo/)) ? true : false;   // active-low: lo == on position
}

// Is the currently-bound function active right now?
function func_active(ctx, func) {
	if (func == "wireguard" || func == "vpn")
		return ctx.popen("ip link show wgclient 2>/dev/null") ? true : false;
	if (func == "openvpn")
		return ctx.popen("ip link show ovpnclient 2>/dev/null") ? true : false;
	if (func == "repeater") {
		let s = ctx.ubus.call("network.interface.internet", "status") ?? {};
		return s.up ? true : false;
	}
	if (func == "wifi") {
		let d = ctx.ubus.call("network.wireless", "status") ?? {};
		for (let r in d)
			for (let i in (d[r].interfaces ?? [])) {
				let cfg = i.config ?? {};
				if (cfg.mode == "ap" && i.ifname) return true;
			}
		return false;
	}
	if (func == "adguardhome")
		return ctx.popen("pgrep -f AdGuardHome 2>/dev/null") ? true : false;
	if (func == "tor")
		return ctx.popen("pgrep -x tor 2>/dev/null") ? true : false;
	return false;
}

export default {
	// Enumerate the functions the mode button can be bound to, as the app's picker
	// (ToggleBtnFuncM: {func, sub_func:[options]}). We advertise what /etc/hotplug.d/
	// button/30-gl-toggle can actually enforce on this device, checking availability.
	get_funcs: function(args, ctx) {
		let c = ctx.uci.cursor();
		let funcs = [];

		// wifi: toggle a radio's broadcast. Offer only the bands with a radio present.
		let bands = [];
		c.foreach("wireless", "wifi-device", function(s) {
			if (s.band == "2g" && index(bands, "2g") < 0) push(bands, "2g");
			if (s.band == "5g" && index(bands, "5g") < 0) push(bands, "5g");
		});
		if (length(bands) > 0) push(funcs, { func: "wifi", sub_func: bands });

		// vpn: the app renders the protocol directly (func "wireguard"/"openvpn"), NOT a
		// "vpn" parent. Offer each protocol we have a saved config for.
		let has = false;
		c.foreach("wireguard", "peers", function(s) { has = true; });
		if (has) push(funcs, { func: "wireguard", sub_func: [] });
		has = false;
		c.foreach("ovpnclient", "clients", function(s) { has = true; });
		if (has) push(funcs, { func: "openvpn", sub_func: [] });

		// repeater: toggle the uplink STA (present whenever a wifi-iface mode=sta exists
		// or can be created -> always offered on this dual-radio device).
		push(funcs, { func: "repeater", sub_func: [] });

		// adguardhome: toggle network-wide ad/tracker blocking, if the binary is present.
		if (fs.access("/usr/bin/AdGuardHome"))
			push(funcs, { func: "adguardhome", sub_func: [] });

		// tor: toggle transparent Tor routing, if the binary is present.
		if (ctx.popen("command -v tor 2>/dev/null"))
			push(funcs, { func: "tor", sub_func: [] });

		// led: toggle the status LEDs, if any are software-controllable.
		let leds = ctx.popen("ls /sys/class/leds 2>/dev/null");
		if (leds && trim(leds) != "") push(funcs, { func: "led", sub_func: [] });

		return { funcs: funcs };
	},

	// Return the currently bound function from our store. func enum values from the Lua:
	// "none", "main_wifi", "guest_wifi", "wifi", "vpn", "cellular", "tor", "adguardhome",
	// "repeater", "led" (plus dynamic vpn tunnel ids). Default "none" when unset.
	get_config: function(args, ctx) {
		let func = read_store(ctx, "func") ?? "none";
		return {
			func: func,
			sub_func: read_store(ctx, "sub_func"),
			listen: (func != "none")            // a function is bound to the switch
		};
	},

	// Whether the physical switch position matches the bound function's live state.
	// sync_status 1 == in sync (switch position and service agree), 0 == out of sync
	// (e.g. switch flipped ON but the VPN isn't up) -> the app offers a "sync" action.
	check_sync_status: function(args, ctx) {
		let func = read_store(ctx, "func") ?? "none";
		if (func == "none") return { sync_status: 1 };
		let want = switch_on(ctx);
		let have = func_active(ctx, func);
		return { sync_status: (want == have) ? 1 : 0, switch_on: want, active: have };
	},

	// Apply the physical switch position to the bound function NOW (the app's "sync"
	// action when out of sync). Reuses the one hotplug handler so enforcement lives in
	// a single place. User-triggered, not polled.
	sync: function(args, ctx) {
		let func = read_store(ctx, "func") ?? "none";
		let sub = read_store(ctx, "sub_func");
		if (func != "none") {
			let action = switch_on(ctx) ? "pressed" : "released";
			ctx.popen(`BUTTON=BTN_0 ACTION=${action} SEEN=1 sh /etc/hotplug.d/button/30-gl-toggle >/dev/null 2>&1`);
		}
		return { func: func, sub_func: sub, listen: (func != "none") };
	},

	// Store the chosen binding. Validates types, writes our own switch-button uci, then
	// returns the fresh config (matching the Lua, which returned M.get_config()).
	// DEGRADE: original validated func against get_funcs() and ran gl_switch_button_check;
	// with no handler scripts that check would reject every func and break the round-trip, and
	// there is no daemon to kick -> we accept any string func/sub_func and DO NOT enforce.
	set_config: function(args, ctx) {
		let func = args.func;
		let sub_func = args.sub_func;

		if (type(func) != "string")
			return ctx.util.E.INVALID_PARAMS;
		if (exists(args, "sub_func") && sub_func != null && type(sub_func) != "string")
			return ctx.util.E.INVALID_PARAMS;

		let c = ctx.uci.cursor();
		// The uci cursor cannot create a config that has no backing file, so touch it first.
		if (!fs.access("/etc/config/switch-button")) {
			let f = fs.open("/etc/config/switch-button", "w");
			if (f) f.close();
		}
		c.set("switch-button", "main", "main");
		c.set("switch-button", "main", "func", func);
		c.delete("switch-button", "main", "sub_func");
		if (type(sub_func) == "string")
			c.set("switch-button", "main", "sub_func", sub_func);
		c.commit("switch-button");

		return {
			func: read_store(ctx, "func") ?? "none",
			sub_func: read_store(ctx, "sub_func")
		};
	}
};
