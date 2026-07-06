// gl-ucode object: timer  (port of decompiled-spec/rpc/timer.lua)
// Scheduled tasks (reboot / led / wifi / disk / screen). GL stored everything in the
// gl_timer uci config and kicked the closed /etc/init.d/gl_timer daemon for enforcement
// -- that daemon is ABSENT on mainline and is DROPPED. We keep gl_timer as our own store
// and supply enforcement ourselves by generating a delimited block in /etc/crontabs/root
// (reboot + led + 2G/5G wifi on/off) and restarting busybox cron.
// DEGRADE (SFT1200): no LCD -> set/get_screen store-only, no enforcement. USB disk power
// gating is board-specific -> set/get_disk store-only. Wifi collapses to plain 2G/5G:
// get_wifi_mlo_support()/6G assumed false, so MLD/6G timer sections are ignored and the
// power_switch (tx-power) branch is stored but not enforced.
'use strict';

const fs = require('fs');

const CRON_FILE = "/etc/crontabs/root";
const CRON_BEGIN = "# BEGIN gl_timer (managed by gl-ucode; do not edit)";
const CRON_END = "# END gl_timer";

// LED enforcement inlined into cron (the closed gl_led daemon is absent). This is a
// DEGRADE: it drops each LED's original trigger and forces default-on / none instead.
const LED_ON = `for l in /sys/class/leds/*; do [ -e "$l/trigger" ] && echo default-on > "$l/trigger" 2>/dev/null; done`;
const LED_OFF = `for l in /sys/class/leds/*; do [ -e "$l/trigger" ] && echo none > "$l/trigger" 2>/dev/null; [ -e "$l/brightness" ] && echo 0 > "$l/brightness" 2>/dev/null; done`;

// ---- validators (ported faithfully from the Lua) ------------------------------------

// is_timer: value must be a 1..256 char decimal string, <= 23 (hour) or <= 59 (min).
function is_timer(v, hour) {
	if (type(v) != "string" || !match(v, /^[0-9]+$/))
		return false;
	if (length(v) < 1 || length(v) > 256)
		return false;
	return int(v) <= (hour ? 23 : 59);
}

// is_week: array whose members are whole numbers 0..6.
function is_week(w) {
	if (type(w) != "array")
		return false;
	for (let d in w) {
		if (type(d) != "int" && type(d) != "double")
			return false;
		if (d != int(d) || d < 0 || d > 6)
			return false;
	}
	return true;
}

function is_wifi_power(p) {
	return p == "Max" || p == "High" || p == "Medium" || p == "Low";
}

// ---- week (de)serialisation ---------------------------------------------------------

function parse_week(s) {
	let out = [];
	for (let p in split(trim(s ?? "0 1 2 3 4 5 6"), /[ ]+/))
		if (match(p, /^[0-9]+$/))
			push(out, int(p));
	return out;
}

// cron day-of-week field: comma list, or "*" when empty.
function cron_dow(weekstr) {
	let w = parse_week(weekstr);
	return length(w) ? join(",", w) : "*";
}

// ---- crontab generator (the mainline replacement for /etc/init.d/gl_timer) ----------

// find the wifi-device (radio) section name whose band matches "2G"/"5G".
function radio_for_band(c, band) {
	let want = lc(band ?? "");
	let all = c.get_all("wireless") ?? {};
	for (let k in all) {
		let s = all[k];
		if (s[".type"] == "wifi-device" && lc(s.band ?? "") == want)
			return s[".name"];
	}
	return null;
}

// build the list of cron lines from the current gl_timer store.
function build_cron(ctx) {
	let c = ctx.uci.cursor();
	let lines = [];

	// reboot
	if (c.get("gl_timer", "reboot", "enable") == "1") {
		let hour = c.get("gl_timer", "reboot", "hour");
		let min = c.get("gl_timer", "reboot", "min");
		if (hour != null && min != null)
			push(lines, sprintf("%s %s * * %s reboot", min, hour, cron_dow(c.get("gl_timer", "reboot", "week"))));
	}

	// led: turnon -> LED_ON, turnoff -> LED_OFF
	if (c.get("gl_timer", "led", "enable") == "1") {
		let dow = cron_dow(c.get("gl_timer", "led", "week"));
		let onh = c.get("gl_timer", "led", "turnon_hour"), onm = c.get("gl_timer", "led", "turnon_min");
		let offh = c.get("gl_timer", "led", "turnoff_hour"), offm = c.get("gl_timer", "led", "turnoff_min");
		if (onh != null && onm != null)
			push(lines, sprintf("%s %s * * %s %s", onm, onh, dow, LED_ON));
		if (offh != null && offm != null)
			push(lines, sprintf("%s %s * * %s %s", offm, offh, dow, LED_OFF));
	}

	// wifi turn_onoff timers (2G/5G, non-guest). DEGRADE: guest ifaces and the
	// power_switch tx-power branch are stored but not cron-enforced on SFT1200.
	let all = c.get_all("gl_timer") ?? {};
	for (let k in all) {
		let s = all[k];
		if (s[".type"] != "timer" || s.enable != "1" || s.func != "turn_onoff")
			continue;
		if (s.guest != null && int(s.guest) == 1)
			continue;
		let radio = radio_for_band(c, s.band);
		if (!radio)
			continue;
		let dow = cron_dow(s.week);
		let up = sprintf("uci -q set wireless.%s.disabled=0; uci -q commit wireless; wifi reload", radio);
		let down = sprintf("uci -q set wireless.%s.disabled=1; uci -q commit wireless; wifi reload", radio);
		if (s.turnon_hour != null && s.turnon_min != null)
			push(lines, sprintf("%s %s * * %s %s", s.turnon_min, s.turnon_hour, dow, up));
		if (s.turnoff_hour != null && s.turnoff_min != null)
			push(lines, sprintf("%s %s * * %s %s", s.turnoff_min, s.turnoff_hour, dow, down));
	}

	return lines;
}

// rewrite the gl_timer-delimited block in /etc/crontabs/root, preserving other lines,
// then (re)start busybox cron.
function write_cron(ctx) {
	let lines = build_cron(ctx);
	let existing = fs.readfile(CRON_FILE) ?? "";
	let kept = [], skip = false;
	for (let ln in split(existing, "\n")) {
		if (ln == CRON_BEGIN) { skip = true; continue; }
		if (ln == CRON_END) { skip = false; continue; }
		if (!skip && length(trim(ln)))
			push(kept, ln);
	}
	let block = [CRON_BEGIN];
	for (let l in lines)
		push(block, l);
	push(block, CRON_END);

	let out = length(kept) ? (join("\n", kept) + "\n") : "";
	out += join("\n", block) + "\n";

	ctx.popen("mkdir -p /etc/crontabs 2>/dev/null");
	fs.writefile(CRON_FILE, out);
	// DEGRADE: original kicked /etc/init.d/gl_timer; we drive stock busybox cron instead.
	ctx.popen("/etc/init.d/cron enable >/dev/null 2>&1; /etc/init.d/cron restart >/dev/null 2>&1");
}

// ---- shared simple-timer (led/reboot/disk/screen) set/get -------------------------

// validate the common {enable, [times], week} shape. `fields`: the two time-field name
// pairs, e.g. ["turnon_hour","turnon_min","turnoff_hour","turnoff_min"] or ["hour","min"].
function validate_simple(args, fields) {
	if (type(args.enable) != "bool")
		return false;
	if (!args.enable)
		return true;
	for (let i = 0; i < length(fields); i += 2) {
		if (!is_timer(args[fields[i]], true))
			return false;
		if (!is_timer(args[fields[i + 1]], false))
			return false;
	}
	return is_week(args.week);
}

// write a simple timer section; when disabled only enable=0 is stored (mirrors the Lua).
function store_simple(ctx, section, args, fields) {
	let c = ctx.uci.cursor();
	if (args.enable) {
		c.set("gl_timer", section, "enable", "1");
		for (let f in fields)
			c.set("gl_timer", section, f, `${args[f]}`);
		c.set("gl_timer", section, "week", join(" ", args.week));
	} else {
		c.set("gl_timer", section, "enable", "0");
	}
	c.commit("gl_timer");
}

function get_simple(ctx, section, fields) {
	let c = ctx.uci.cursor();
	let res = { enable: (c.get("gl_timer", section, "enable") == "1") };
	for (let f in fields)
		res[f] = c.get("gl_timer", section, f) ?? "00";
	res.week = parse_week(c.get("gl_timer", section, "week"));
	return res;
}

const F_ON_OFF = ["turnon_hour", "turnon_min", "turnoff_hour", "turnoff_min"];
const F_REBOOT = ["hour", "min"];

// ---- wifi timer validation / storage ------------------------------------------------

function valid_wifi(w) {
	if (type(w) != "object")
		return false;
	if (type(w.enable) != "bool")
		return false;
	if (int(w.guest) != 0 && int(w.guest) != 1)
		return false;
	if (w.band != "5G" && w.band != "2G" && w.band != "6G" && w.band != "MLD")
		return false;
	if (type(w.func) != "string" || (w.func != "turn_onoff" && w.func != "power_switch"))
		return false;
	if (w.turnon_hour != null && !is_timer(w.turnon_hour, true)) return false;
	if (w.turnon_min != null && !is_timer(w.turnon_min, false)) return false;
	if (w.turnoff_hour != null && !is_timer(w.turnoff_hour, true)) return false;
	if (w.turnoff_min != null && !is_timer(w.turnoff_min, false)) return false;
	if (w.switch_hour != null && !is_timer(w.switch_hour, true)) return false;
	if (w.switch_min != null && !is_timer(w.switch_min, false)) return false;
	if (w.restore_hour != null && !is_timer(w.restore_hour, true)) return false;
	if (w.restore_min != null && !is_timer(w.restore_min, false)) return false;
	if (w.switch_power != null && !is_wifi_power(w.switch_power)) return false;
	if (w.restore_power != null && !is_wifi_power(w.restore_power)) return false;
	if (w.week != null && !is_week(w.week)) return false;
	return true;
}

// deterministic gl_timer section name for a wifi timer (must contain "wifi" so get_wifi
// surfaces it; guest/band prefixes match the original naming scheme).
function wifi_section(band, guest) {
	let prefix = lc(band ?? "");
	return int(guest) == 1 ? `${prefix}guestwifi` : `${prefix}wifi`;
}

// ---- exported methods ---------------------------------------------------------------

export default {
	set_led: function(args, ctx) {
		if (!validate_simple(args, F_ON_OFF))
			return ctx.util.E.INVALID_PARAMS;
		store_simple(ctx, "led", args, F_ON_OFF);
		write_cron(ctx);
		return {};
	},

	get_led: function(args, ctx) {
		return get_simple(ctx, "led", F_ON_OFF);
	},

	set_reboot: function(args, ctx) {
		if (!validate_simple(args, F_REBOOT))
			return ctx.util.E.INVALID_PARAMS;
		store_simple(ctx, "reboot", args, F_REBOOT);
		write_cron(ctx);
		return {};
	},

	get_reboot: function(args, ctx) {
		return get_simple(ctx, "reboot", F_REBOOT);
	},

	set_wifi: function(args, ctx) {
		let wifi = args.wifi;
		if (type(wifi) != "array")
			return ctx.util.E.INVALID_PARAMS;
		let c = ctx.uci.cursor();
		for (let w in wifi) {
			if (!valid_wifi(w))
				return ctx.util.E.INVALID_PARAMS;
			let name = wifi_section(w.band, w.guest);
			c.set("gl_timer", name, "timer");   // ensure the section exists
			for (let k in w) {
				let v = w[k];
				if (k == "week") {
					c.set("gl_timer", name, "week", join(" ", v));
				} else {
					if (k == "func")
						c.set("gl_timer", "global", `${w.band}_func`, `${v}`);
					if (k == "enable")
						c.set("gl_timer", name, "enable", v ? "1" : "0");
					else
						c.set("gl_timer", name, k, `${v}`);
				}
			}
		}
		c.commit("gl_timer");
		write_cron(ctx);
		return {};
	},

	get_wifi: function(args, ctx) {
		let c = ctx.uci.cursor();
		let wifi = [];
		let all = c.get_all("gl_timer") ?? {};
		for (let k in all) {
			let s = all[k];
			if (s[".type"] != "timer")
				continue;
			let name = s[".name"];
			// DEGRADE: SFT1200 has no MLO/6G radio -> skip those timer sections.
			if (index(name, "mld") >= 0 || index(name, "6g") >= 0)
				continue;
			if (index(name, "wifi") < 0)
				continue;
			let week = parse_week(s.week);
			if (s.func == "turn_onoff")
				push(wifi, {
					enable: s.enable == "1", func: s.func, band: s.band, guest: int(s.guest ?? 0),
					turnon_hour: s.turnon_hour, turnon_min: s.turnon_min,
					turnoff_hour: s.turnoff_hour, turnoff_min: s.turnoff_min, week: week
				});
			else if (s.func == "power_switch")
				push(wifi, {
					enable: s.enable == "1", func: s.func, band: s.band, guest: int(s.guest ?? 0),
					switch_hour: s.switch_hour, switch_min: s.switch_min, switch_power: s.switch_power,
					restore_hour: s.restore_hour, restore_min: s.restore_min, restore_power: s.restore_power, week: week
				});
		}
		return {
			wifi: wifi,
			wifi_func_2g: c.get("gl_timer", "global", "2G_func"),
			wifi_func_5g: c.get("gl_timer", "global", "5G_func")
			// DEGRADE: wifi_func_6g / wifi_func_mld omitted (no 6G/MLO on SFT1200)
		};
	},

	set_disk: function(args, ctx) {
		if (!validate_simple(args, F_ON_OFF))
			return ctx.util.E.INVALID_PARAMS;
		// DEGRADE: USB disk power gating is board-specific and absent on SFT1200 -> store only.
		store_simple(ctx, "disk", args, F_ON_OFF);
		return {};
	},

	get_disk: function(args, ctx) {
		return get_simple(ctx, "disk", F_ON_OFF);
	},

	set_screen: function(args, ctx) {
		if (!validate_simple(args, F_ON_OFF))
			return ctx.util.E.INVALID_PARAMS;
		// DEGRADE: no LCD on SFT1200 -> accept + store, but there is nothing to enforce.
		store_simple(ctx, "screen", args, F_ON_OFF);
		return {};
	},

	get_screen: function(args, ctx) {
		return get_simple(ctx, "screen", F_ON_OFF);
	}
};
