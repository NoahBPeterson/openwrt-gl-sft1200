// gl-ucode object: led  (port of decompiled-spec/rpc/led.lua, 4.3.25-only)
// Global LED on/off master switch. GL wrote gl_led.global.led_enable and kicked
// /etc/init.d/gl_led (a closed shell daemon absent on mainline) -- DROPPED. Enforcement
// is reimplemented over the Linux LED subsystem (/sys/class/leds/*) via ctx.popen; the
// gl_led.global.led_enable uci key is kept as the store so get/set_config round-trip.
'use strict';

const fs = require('fs');

// list of LED device names under /sys/class/leds (fs.lsdir, popen fallback)
function led_names(ctx) {
	let d = fs.lsdir("/sys/class/leds");
	if (type(d) == "array") return d;
	let out = ctx.popen("ls /sys/class/leds 2>/dev/null");
	return (type(out) == "string" && length(out)) ? split(out, "\n") : [];
}

// active trigger for a led, parsed from the "[bracketed]" token in its trigger file
function active_trigger(name) {
	let d = fs.readfile(`/sys/class/leds/${name}/trigger`);
	if (!d) return null;
	let m = match(d, /\[([^\]]+)\]/);
	return m ? m[1] : null;
}

// uci section name for a led (colons/other chars are illegal in uci section names)
function led_section(name) {
	return "led_" + replace(name, /[^A-Za-z0-9_]/g, "_");
}

// Enforce the master switch across every kernel LED.
// disable: remember the current trigger, then trigger=none + brightness=0.
// enable:  restore the saved trigger (default-on if none was saved) which relights it.
function apply_leds(ctx, enable) {
	let c = ctx.uci.cursor();
	for (let name in led_names(ctx)) {
		let base = `/sys/class/leds/${name}`;
		let sec = led_section(name);
		if (enable) {
			let saved = c.get("gl_led", sec, "trigger") ?? "default-on";
			ctx.popen(`echo '${saved}' > '${base}/trigger' 2>/dev/null`);
		} else {
			let cur = active_trigger(name);
			if (cur && cur != "none") {
				c.set("gl_led", sec, "led", name);
				c.set("gl_led", sec, "trigger", cur);
			}
			ctx.popen(`echo none > '${base}/trigger' 2>/dev/null; echo 0 > '${base}/brightness' 2>/dev/null`);
		}
	}
	c.commit("gl_led");
}

export default {
	set_config: function(args, ctx) {
		let led_enable = args.led_enable;
		if (type(led_enable) != "bool")
			return ctx.util.E.INVALID_PARAMS;

		let c = ctx.uci.cursor();
		c.set("gl_led", "global", "led_enable", led_enable ? "1" : "0");
		c.commit("gl_led");

		// DEGRADE: original restarted the closed /etc/init.d/gl_led daemon; on mainline
		// we enforce the state directly over the kernel LED subsystem instead.
		apply_leds(ctx, led_enable);
		return {};
	},

	get_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		return { led_enable: (c.get("gl_led", "global", "led_enable") == "1") };
	}
};
