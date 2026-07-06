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

export default {
	// Enumerate the functions the switch can be bound to. On mainline there are no
	// /etc/gl-switch.d handler scripts to drive, so nothing is bindable.
	// DEGRADE: original scanned /etc/gl-switch.d and cross-read wireless/route_policy/modem;
	// all absent here -> empty list. Field name/shape ({funcs:[{func,sub_func?}]}) preserved.
	get_funcs: function(args, ctx) {
		return { funcs: [] };
	},

	// Return the currently bound function from our store. func enum values from the Lua:
	// "none", "main_wifi", "guest_wifi", "wifi", "vpn", "cellular", "tor", "adguardhome",
	// "repeater", "led" (plus dynamic vpn tunnel ids). Default "none" when unset.
	get_config: function(args, ctx) {
		return {
			func: read_store(ctx, "func") ?? "none",
			sub_func: read_store(ctx, "sub_func")
		};
	},

	// Whether the physical switch position matches the web/service state.
	// DEGRADE: the SF1688 exposes no usable switch and the per-func web-state cross-reads
	// (vpn/tor/adguardhome/wifi/repeater/cellular/led) were dropped, so there is nothing to
	// compare -> always in sync. Field name (sync_status) preserved.
	check_sync_status: function(args, ctx) {
		return { sync_status: 1 };
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
		// ensure the section exists (creates /etc/config/switch-button on commit if absent)
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
