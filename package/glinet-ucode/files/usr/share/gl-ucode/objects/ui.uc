// gl-ucode object: ui  (port of decompiled-spec/rpc/ui.lua)
// The app hits ui.check_initialized / ui.get_lang first (both no-auth), then
// either ui.init (onboarding) or the login flow.
'use strict';

import { set_verifier } from '../lib/session.uc';

const fs = require('fs');

const LOCALES_PATH = "/www/i18n";
const MENU_PATH    = "/www/menus";

/**
 * @param {string} p
 */
function readfile(p) { let d = fs.readfile(p); return d ? d : null; } // (variable) d: string | null // second `d`: (variable) d: string // `readfile`: Returns: unknown | null

export default {
	check_initialized: function(args, ctx) {
		let c = ctx.uci.cursor();
		let inited = (c.get("oui-httpd", "main", "inited") == "1");
		let model = (function() { let d = fs.readfile("/proc/gl-hw-info/model");
			return d ? trim(d) : (c.get("board_special", "hardware", "model") ?? "sft1200"); })();
		let fwcat = (function() { let d = fs.readfile("/etc/firmware_type"); return d ? trim(d) : "2c"; })();
		// the app reads the device MAC here to identify/manage the router; empty -> it
		// refuses with "Device Mac is empty". Source it from the factory config (mtd) or
		// the LAN device address.
		let fmac = c.get("factory", "device", "mac")
			?? (function() { let a = fs.readfile("/sys/class/net/br-lan/address") ?? fs.readfile("/sys/class/net/eth0/address"); return a ? uc(trim(a)) : ""; })();
		return {
			initialized: inited,
			model: model,
			mac: fmac,
			factory_mac: fmac,
			macaddr: fmac,
			firmware_category: fwcat,
			support_online_upgrade: false,
			support_screen_init: false,
			environment_support: false
		};
	},

	get_lang: function(args, ctx) {
		let seen = {}, list = [];
		let dir = fs.access(LOCALES_PATH) ? fs.lsdir(LOCALES_PATH) : null; // (variable) dir: unknown | null // `lsdir`: Returns: array<string> | null 
		for (let name in (dir ?? [])) {
			let m = match(name, /\.([a-zA-Z-]+)\.json$/);
			if (m && !seen[m[1]]) {
				seen[m[1]] = true;
				push(list, { value: m[1], label: m[1], id: length(list) + 1 });
			}
		}
		if (length(list) == 0)
			list = [ { value: "en", label: "English", id: 1 } ];
		let c = ctx.uci.cursor();
		return { lang_list: list, langs: list, lang: c.get("oui-httpd", "main", "lang") ?? "en" };
	},

	/**
	* @param {object} args
	*/
	load_locales: function(args, ctx) {
		// lang feeds a regex below, so constrain it (avoid regex-injection)
		if (type(args.lang) != "string")
			args.lang = "en";
		let lang = (args && args.lang && match(args.lang, /^[a-zA-Z-]+$/)) ? args.lang : "en";
		let merged = {};
		let dir = fs.access(LOCALES_PATH) ? fs.lsdir(LOCALES_PATH) : null;
		let re = regexp(`\\.${lang}\\.json$`);   // ucode match() needs a regex, not a string
		for (let name in (dir ?? [])) {
			if (!match(name, re)) continue;
			let raw = readfile(`${LOCALES_PATH}/${name}`);
			if (!raw) continue;
			let obj;
			try { obj = json(raw); } catch (e) { continue; }
			for (let k in obj) merged[k] = obj[k];
		}
		return merged;
	},

	get_menu_list: function(args, ctx) {
		let menus = [];
		let dir = fs.access(MENU_PATH) ? fs.lsdir(MENU_PATH) : null;
		for (let name in (dir ?? [])) {
			if (!match(name, /\.json$/)) continue;
			let raw = readfile(`${MENU_PATH}/${name}`);
			if (!raw) continue;
			try { push(menus, json(raw)); } catch (e) {}
		}
		return { menus: menus };
	},

	get_timezone_list: function(args, ctx) {
		// best-effort: zoneinfo names from the system tz database
		let out = ctx.popen("find /usr/share/zoneinfo -type f 2>/dev/null | sed 's|/usr/share/zoneinfo/||' | sort");
		if (type(out) != "string") return { timezones: [] };   // narrow string|null -> string
		return { timezones: split(out, "\n") };
	},

	// onboarding: set admin password + mark initialized
	/**
	* @param {object} args
	*/
	init: function(args, ctx) {
		let c = ctx.uci.cursor();
		if (c.get("oui-httpd", "main", "inited") == "1")
			return ctx.util.E.ACCESS;
		let username = args.username, password = args.password, lang = args.lang;
		if (type(username) != "string" || type(password) != "string" || type(lang) != "string")
			return ctx.util.E.INVALID_PARAMS;
		if (length(password) < 5)
			return { err_code: 20023005, err_msg: "password too weak" };
		// Set the shadow password. MUST be md5crypt ($1$): the GL app reproduces the
		// full shadow crypt field client-side (sha256(user:field:nonce)) and only supports
		// md5crypt — a bcrypt/sha512 hash makes every login fail with "password incorrect".
		let p = fs.popen(`chpasswd -c MD5 2>/dev/null`, "w");
		if (p) { p.write(`${username}:${password}\n`); p.close(); }
		// store the sha256 password verifier used by the GL-app challenge/login bridge
		set_verifier(username, password);
		// The uci cursor can't create a config file that doesn't exist yet, so ensure
		// /etc/config/oui-httpd is present, then create the named 'main' section before
		// writing options (mirrors the factory-init shim). Without this the commit is a
		// silent no-op and `inited` never persists -> the app loops on setup.
		if (!fs.access("/etc/config/oui-httpd")) {
			let t = fs.open("/etc/config/oui-httpd", "w");
			if (t) t.close();
		}
		c.set("oui-httpd", "main", "main");
		c.set("oui-httpd", "main", "lang", lang);
		c.set("oui-httpd", "main", "inited", "1");
		c.commit("oui-httpd");
		return { username: username };
	}
};
