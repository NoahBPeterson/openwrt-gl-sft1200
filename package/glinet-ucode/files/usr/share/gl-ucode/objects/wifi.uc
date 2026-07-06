// gl-ucode object: wifi  (port of decompiled-spec/rpc/wifi.lua — ~3538 LoC)
// App-critical surface for the dashboard + WiFi-settings screen, sourced from
// stock uci `wireless` (wifi-device/wifi-iface) + ubus iwinfo/hostapd/network.wireless.
// Siflower SF1688 is mac80211 + hostapd, so every mtk / mtkwifi / qcawificfg80211 /
// qcacld32 / mwctl / iwpriv branch of the original is DROPPED, and GL-only helpers
// (lfactory, /proc/gl-hw-info, gl.wifi feature files, gl-cloud, gl_screen, gl-mesh,
// MLO/6GHz) are DEGRADEd to sane defaults with the SAME field names the app expects.
'use strict';

const fs = require('fs');

// ---- module-private helpers --------------------------------------------------

// param-type guards mirroring gl.validator.base(...)
function is_opt_bool(v)  { return v == null || type(v) == "bool"; }
function is_opt_int(v, min) {
	if (v == null) return true;
	if (type(v) != "int" && type(v) != "double") return false;
	if (int(v) != v) return false;               // must be integral
	if (min != null && v < min) return false;
	return true;
}
function is_opt_str(v, min, max) {
	if (v == null) return true;
	if (type(v) != "string") return false;
	if (min != null && length(v) < min) return false;
	if (max != null && length(v) > max) return false;
	return true;
}
// gl.validator.wpakey: 64 hex chars, OR length 8..63
function wpakey(v) {
	if (type(v) != "string") return false;
	if (length(v) == 64) return (match(v, /^[0-9a-fA-F]+$/) != null);
	return length(v) > 7 && length(v) < 64;
}

// collect wifi-device / wifi-iface sections from uci `wireless`
function wireless_all(c) {
	let all = c.get_all("wireless") ?? {};
	let devs = [], ifaces = [];
	for (let k in all) {
		let s = all[k];
		if (s[".type"] == "wifi-device") push(devs, s);
		else if (s[".type"] == "wifi-iface") push(ifaces, s);
	}
	return { devs: devs, ifaces: ifaces };
}

// per-radio runtime from netifd: { <device>: { up, ifname, channel } }
function radio_runtime(ctx) {
	let out = {};
	let dump = ctx.ubus.call("network.wireless", "status") ?? {};
	for (let radio in dump) {
		let r = dump[radio];
		let ifname = null;
		for (let i in (r.interfaces ?? []))
			if (i.ifname) { ifname = i.ifname; break; }
		out[radio] = { up: r.up ? true : false, ifname: ifname,
		               channel: (r.config && r.config.channel) ? r.config.channel : null };
	}
	return out;
}

function iwinfo_info(ctx, ifname) {
	if (!ifname) return {};
	return ctx.ubus.call("iwinfo", "info", { device: ifname }) ?? {};
}

function device_phyname(ctx, device, ifname) {
	let info = iwinfo_info(ctx, ifname);
	if (info.phyname) return info.phyname;
	// fall back: uci radio names on this device are already phyN; else radioN->phyN
	return replace(device, "radio", "phy");
}

// standard 5 GHz DFS channels (mac80211 / ETSI+FCC). gl.wifi.channel_is_dfs is absent.
function channel_is_dfs(channel) {
	let dfs = [52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,144];
	for (let ch in dfs) if (ch == channel) return true;
	return false;
}

// port of channel_is_allowed(country, band, channel, mhz)
function channel_is_allowed(country, band, channel, mhz) {
	if (band == "2g") {
		if (country == "JP") return mhz < 2484 && mhz > 2401;
		return mhz < 2495 && mhz > 2401;
	} else if (band == "5g") {
		if (country == "DE") return mhz > 5150 && mhz <= 5825 && mhz != 5720;
		return mhz > 5150 && mhz < 5925;
	} else if (band == "6g") {
		return mhz > 5925 && mhz < 7125;
	}
	return true;
}

const PSC_CHANNELS = [5,21,37,53,69,85,101,117,133,149,165,181,197,213,229];
function channel_is_psc(channel) {
	if (channel == "auto") return true;
	for (let p in PSC_CHANNELS) if (p == channel) return true;
	return false;
}

// DEGRADE: gl.wifi feature files (/usr/share|/var/run/wifi-features-<dev>) are absent on
// mainline; synthesise capabilities from live iwinfo, fall back to Wi-Fi 6 defaults.
function get_features(ctx, band, ifname) {
	let info = iwinfo_info(ctx, ifname);
	let htmodes = info.htmodes ?? [];
	let has = function(pfx) { for (let h in htmodes) if (index(h, pfx) == 0) return true; return false; };
	let hasVHT = has("VHT"), hasHE = has("HE"), hasEHT = has("EHT");
	let b2g = (band == "2g");
	return {
		hwmodes: { n: true, g: b2g, ac: (hasVHT || (!b2g && length(htmodes) == 0)),
		           ax: (hasHE || length(htmodes) == 0), be: hasEHT },
		max_vht: has("VHT160") ? 160 : 80,
		max_he:  b2g ? 40 : (has("HE160") ? 160 : 80),
		max_eht: b2g ? 40 : 160,
		max_txpwr: 23,                 // DEGRADE: no per-country txpwr table; nominal cap
		phy: info.phyname ?? null
	};
}

// map a level name to a dBm value (DEGRADE of the feature-file txpwrs tables → linear)
function level_to_dbm(maxtxpwr, level) {
	let f = 0.5;
	if (level == "Max") f = 1.0;
	else if (level == "High") f = 0.7;
	else if (level == "Medium") f = 0.5;
	else if (level == "Low") f = 0.3;
	let v = maxtxpwr * f;
	return int(v) + ((v > int(v)) ? 1 : 0);      // ceil
}
function dbm_to_level(maxtxpwr, dbm) {
	if (dbm == null) return "Max";
	dbm = +dbm;
	if (dbm >= maxtxpwr) return "Max";
	if (dbm >= maxtxpwr * 0.7) return "High";
	if (dbm >= maxtxpwr * 0.5) return "Medium";
	return "Low";
}

// hostapd phy state (was get_phy_state via raw ctrl socket → ubus hostapd get_status)
function phy_state(ctx, ifname) {
	if (!ifname) return null;
	let st = ctx.ubus.call(`hostapd.${ifname}`, "get_status");
	return (st && st.state) ? st.state : null;
}

// generate a locally-administered unicast MAC (was get_random_bssid via /dev/urandom).
// First octet fixed to 02 (locally-administered, unicast); last 5 octets random. No hex
// parsing in ucode -- shell emits the tail as 10 hex chars.
function random_bssid(ctx) {
	let hex = ctx.popen("od -An -tx1 -N5 /dev/urandom | tr -d ' \\n'");
	if (type(hex) != "string" || length(hex) < 10) return null;
	return uc(sprintf("02:%s:%s:%s:%s:%s",
		substr(hex, 0, 2), substr(hex, 2, 2), substr(hex, 4, 2), substr(hex, 6, 2), substr(hex, 8, 2)));
}

// ---- get_config helpers ------------------------------------------------------

// per-radio hwmode display list (array), ported from get_config band tables + ax/be augmentation
function build_hwmodelist(band, features) {
	let list;
	if (band == "6G") list = ["11ax/be"];
	else if (band == "5G") list = ["11ac", "11n/ac", "11a/n/ac"];
	else list = ["11n", "11g/n", "11b/g/n"];
	if (features.hwmodes.ax && band != "6G") {
		let tail = list[length(list) - 1];
		let out = [];
		for (let m in list) push(out, m + "/ax");
		push(out, tail);
		list = out;
	}
	// DEGRADE: 802.11be augmentation dropped (no EHT hardware on SF1688)
	return list;
}

// per-hwmode max-bandwidth map (htmodelist), ported
function build_htmodelist(band, hwmodelist, features) {
	let out = {};
	for (let m in hwmodelist) {
		let max = 40;
		if (index(m, "be") >= 0) max = features.max_eht;
		else if (index(m, "ax") >= 0) max = features.max_he;
		else if (index(m, "ac") >= 0) max = features.max_vht;
		out[m] = max;
	}
	out.auto = (band == "2G");
	return out;
}

// ported hwmode-derivation (mac80211 branch, lines 864-883)
function derive_hwmode(s, band) {
	let hwmode;
	let hw = s.hwmode ?? (band == "2G" ? "11g" : "11a");
	if (hw == "11g") {
		if (s.require_mode == "n") hwmode = "11n";
		else if (s.legacy_rates == "1") hwmode = "11b/g/n";
		else hwmode = "11g/n";
	} else if (s.require_mode == "ac") hwmode = "11ac";
	else if (s.require_mode == "n") hwmode = "11n/ac";
	else hwmode = "11a/n/ac";
	if (s.htmode && index(s.htmode, "HE") >= 0) hwmode += "/ax";
	return hwmode;
}

// ported htmode-display derivation (mac80211 branch, lines 748-773 + auto rule 884-886)
function derive_htmode_display(s, band, features) {
	let noscan = s.noscan ?? "0";
	let hw = s.hwmode ?? (band == "2G" ? "11g" : "11a");
	let htmode, max_bw = 40;
	if (hw == "11b") htmode = 20;
	else if (!s.htmode || s.htmode == "NOHT") htmode = 20;
	else {
		let mn = match(s.htmode, /([A-Za-z]+)([0-9]+)/);
		if (mn) {
			if (mn[1] == "VHT") max_bw = features.max_vht;
			else if (mn[1] == "HE") max_bw = features.max_he;
			htmode = int(mn[2]);
		} else htmode = 20;
	}
	if (band == "2G" && noscan == "0" && htmode == max_bw) return "auto";
	return `${htmode}`;
}

// ported get_wifi_iface_config for mac80211 (factory-default branch DEGRADEd: init=true)
function iface_config(ifaces, device) {
	let out = [], real_htmode = null;
	for (let s in ifaces) {
		if (s.device != device) continue;
		if ((s.mode ?? "ap") != "ap") continue;
		if (s.multi_ap == "1") continue;
		let enabled = (s.disabled != "1");
		push(out, {
			enabled: enabled,
			ssid: s.ssid ?? "",
			hidden: (s.hidden == "1"),
			encryption: s.encryption ?? "none",
			key: s.key ?? "",
			guest: (s.guest == "1" || s.network == "guest"),
			bssid: s.macaddr ?? "",
			name: s[".name"],
			iot: (s.iot == "1" || s.network == "iot"),
			random_bssid: (s.random_bssid == "1"),
			// DEGRADE: GL factory-provision state (lfactory/gl-hw-info) absent -> always "configured"
			init: true
		});
	}
	return [out, real_htmode];
}

// ---- set_config helpers ------------------------------------------------------

// set a uci option from param[name], coercing bool -> "1"/"0" (skips nil)
function set_opt(c, section, name, value) {
	if (value == null) return;
	if (type(value) == "bool") value = value ? 1 : 0;
	c.set("wireless", section, name, `${value}`);
}

// ported set_htmode (mac80211 only): writes device hwmode/htmode/noscan, sets param.require_mode/legacy_rates
function set_htmode(c, device, param, band) {
	let htmode = param.htmode, hwmode = param.hwmode;
	if (htmode == null || hwmode == null) return;
	let valid_ht = { "20": true, "40": true, "80": true, "160": true, "320": true, "auto": true };
	if (!valid_ht[`${htmode}`]) return;

	let ht_prefix = "HT";
	if (index(hwmode, "be") >= 0) ht_prefix = "EHT";
	else if (index(hwmode, "ax") >= 0) ht_prefix = "HE";
	else if (index(hwmode, "ac") >= 0) ht_prefix = "VHT";

	let require_mode = "", legacy_rates = 0;
	if (index(hwmode, "ac") >= 0) {
		if (index(hwmode, "n") < 0) require_mode = "ac";
		else if (index(hwmode, "11a/") < 0) require_mode = "n";
		hwmode = "11a";
	} else {
		if (index(hwmode, "g") < 0) require_mode = "n";
		else if (index(hwmode, "b") >= 0) legacy_rates = 1;
		hwmode = "11g";
	}
	param.require_mode = require_mode;
	param.legacy_rates = legacy_rates;

	let is2g = (band == "2g");
	if (htmode == "auto") {
		c.set("wireless", device, "htmode", ht_prefix + "40");
		c.set("wireless", device, "noscan", "0");
	} else {
		let n = int(htmode);
		if (is2g && n == 40) c.set("wireless", device, "noscan", "1");
		else if (is2g) c.set("wireless", device, "noscan", "0");
		c.set("wireless", device, "htmode", ht_prefix + htmode);
	}
	c.set("wireless", device, "hwmode", hwmode);
}

// lightweight channel_check (DEGRADE of the per-country valid_channels table): range +
// membership in the live freqlist when available.
function channel_check(ctx, ifname, channel, band) {
	if (channel == "auto" || channel == 0) return true;
	let ch = int(channel);
	if (band == "2g" && (ch < 1 || ch > 14)) return false;
	if (band == "5g" && (ch < 36 || ch > 177)) return false;
	if (band == "6g" && (ch < 1 || ch > 233)) return false;
	let fl = ctx.ubus.call("iwinfo", "freqlist", { device: ifname });
	let results = (fl && fl.results) ? fl.results : null;
	if (!results || !length(results)) return true;      // no live data -> accept range
	for (let f in results) if (f.channel == ch && !f.restricted) return true;
	return false;
}

// ported set_random_bssid for mac80211 (non-6g, non-qca). Returns true if a restart is needed.
function set_random_bssid(c, ctx, iface, on) {
	if (on == null) return false;
	let cur = c.get("wireless", iface, "random_bssid");
	if ((cur == "1") == on) return false;
	c.set("wireless", iface, "random_bssid", on ? "1" : "0");
	if (on) {
		let mac = random_bssid(ctx);
		if (mac) c.set("wireless", iface, "macaddr", mac);
	} else {
		let fmac = c.get("wireless", iface, "factory_macaddr");
		if (fmac) c.set("wireless", iface, "macaddr", fmac);
	}
	return true;
}

// ported setup_network_by_type: toggle network.<type>.disabled to match iface enablement
function setup_network_by_type(c, network_type) {
	if (c.get("network", network_type, "disabled") == null) return;   // section absent -> skip
	let disabled = "1";
	let all = c.get_all("wireless") ?? {};
	for (let k in all) {
		let s = all[k];
		if (s[".type"] == "wifi-iface" && s.network == network_type && s.disabled != "1") { disabled = "0"; break; }
	}
	if (c.get("network", network_type, "disabled") != disabled) {
		c.set("network", network_type, "disabled", disabled);
		c.commit("network");
	}
}

// apply txpower for mac80211 (shared by set_config + set_txpower)
function apply_txpower(ctx, c, device, level, channel) {
	let rt = radio_runtime(ctx);
	let ifname = (rt[device] && rt[device].ifname) ? rt[device].ifname : null;
	let band = c.get("wireless", device, "band") ?? "";
	let features = get_features(ctx, band, ifname);
	let dbm = level_to_dbm(features.max_txpwr, level);
	c.set("wireless", device, "txpower", `${dbm}`);
	c.commit("wireless");
	let phy = features.phy ?? device_phyname(ctx, device, ifname);
	// iw expects mBm (dBm * 100)
	ctx.popen(`iw phy ${phy} set txpower fixed ${dbm * 100} 2>/dev/null`);
}

export default {
	// Dashboard radio state: { res: [ { name, state, channel } ] }
	get_status: function(args, ctx) {
		let c = ctx.uci.cursor();
		let rt = radio_runtime(ctx);
		let res = [];
		let devs = wireless_all(c).devs;
		for (let s in devs) {
			let name = s[".name"];
			let r = rt[name] ?? {};
			let status = { name: name, state: "starting" };
			if (r.up) {
				let state = phy_state(ctx, r.ifname);
				let info = iwinfo_info(ctx, r.ifname);
				if (state != "DFS") {
					status.state = "ready";
					status.channel = info.channel ?? r.channel;
				}
			}
			push(res, status);
		}
		return { res: res };
	},

	// Full WiFi-settings payload: { bandmode, dfs_support, res: [ per-radio ] }
	get_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let rt = radio_runtime(ctx);
		let w = wireless_all(c);
		let devs = w.devs, ifaces = w.ifaces;
		let dfs_support = fs.access("/proc/gl-hw-info/dfs") ? true : false;   // DEGRADE: absent -> false
		let res = [];

		for (let s in devs) {
			let device = s[".name"];
			let bandlc = s.band ?? "";
			let band = uc(bandlc);
			let country = s.country;                         // DEGRADE: lfactory country -> uci ?? US
			let ifname = (rt[device] && rt[device].ifname) ? rt[device].ifname : null;
			let features = get_features(ctx, bandlc, ifname);

			let hwmodelist = build_hwmodelist(band, features);
			let htmodelist = build_htmodelist(band, hwmodelist, features);
			let hwmode = derive_hwmode(s, band);
			let htmode = derive_htmode_display(s, band, features);

			// channel list from live freqlist
			let channels = [];
			let fl = ctx.ubus.call("iwinfo", "freqlist", { device: ifname });
			let results = (fl && fl.results) ? fl.results : [];
			for (let freq in results) {
				let ch = freq.channel;
				if (freq.restricted) continue;
				if (!channel_is_allowed(country, bandlc, ch, freq.mhz)) continue;
				let dfs = (bandlc == "5g") ? channel_is_dfs(ch) : false;
				let psc = (bandlc == "6g") ? channel_is_psc(ch) : false;
				if (dfs && !dfs_support) continue;
				push(channels, { channel: ch, dfs: dfs, psc: psc });
			}

			let encryptions = (bandlc == "6g")
				? ["sae"]
				: ["none", "psk2", "psk-mixed", "sae", "sae-mixed"];

			let iflist = iface_config(ifaces, device)[0];

			// txpower level from stored uci value (DEGRADE: no feature txpwr table)
			let stored = s.ctp ?? s.txpower;
			let maxtxpwr = features.max_txpwr;
			let txpower = dbm_to_level(maxtxpwr, stored);

			let channel = (!s.channel || s.channel == "auto") ? 0 : int(s.channel);

			let ret = {
				hwmodes: hwmodelist,
				htmodes: htmodelist,
				channels: channels,
				encryptions: encryptions,
				band: band,
				hwmode: hwmode,
				htmode: `${htmode}`,
				channel: channel,
				txpower: txpower,
				device: device,
				ifaces: iflist
			};
			if (s.chan_6g_only_psc != null) ret.chan_6g_only_psc = (s.chan_6g_only_psc == "1");
			push(res, ret);
		}

		// DEGRADE: bandmode only meaningful on qcacld32 (band-mutex hardware) -> empty
		return { bandmode: {}, dfs_support: dfs_support, res: res };
	},

	get_environment_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let use = c.get("environment", "global", "use_environment");   // DEGRADE: config may be absent
		return { use_environment: (use == "indoor") ? 1 : 0 };
	},

	set_environment_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		if (type(args.use_environment) != "int" && type(args.use_environment) != "double")
			return ctx.util.E.INVALID_PARAMS;              // invalid use_environment
		let use;
		if (args.use_environment == 1) use = "indoor";
		else if (args.use_environment == 0) use = "outdoor";
		else return ctx.util.E.INVALID_PARAMS;             // invalid use_environment
		if (c.get("environment", "global", "use_environment") == use) return {};
		c.set("environment", "global", "use_environment", use);
		c.commit("environment");
		ctx.popen("(sleep 1; /sbin/wifi reload) >/dev/null 2>&1 &");
		return {};
	},

	// Change SSID / password / channel / enable / htmode / encryption for a radio or iface.
	set_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let param = args ?? {};
		let iface_name = param.iface_name;
		let device = param.device;
		let random_bssid_p = param.random_bssid;

		// ---- validation (ported) ----
		if (iface_name != null && !is_opt_str(iface_name, 1, 256))
			return ctx.util.E.INVALID_PARAMS;              // invalid iface_name
		if (iface_name && !c.get("wireless", iface_name))
			return ctx.util.E.INVALID_PARAMS;              // invalid iface_name
		if (!is_opt_bool(param.enabled))
			return ctx.util.E.INVALID_PARAMS;              // invalid enabled

		// DEGRADE: TAP-S2S / ovpnclient guest-lock check dropped (no gl ovpn on mainline)

		if (!is_opt_str(param.ssid, 1, 32))
			return ctx.util.E.INVALID_PARAMS;              // invalid ssid
		if (!is_opt_bool(param.hidden))
			return ctx.util.E.INVALID_PARAMS;              // invalid hidden
		if (param.key != null) {
			if (!wpakey(param.key))
				return ctx.util.E.INVALID_PARAMS;          // invalid key
			if (length(param.key) == 64 && param.encryption && index(param.encryption, "sae") >= 0)
				return ctx.util.E.INVALID_PARAMS;          // invalid key (64-hex not allowed with sae)
		}
		if (device != null && !is_opt_str(device, 1, 256))
			return ctx.util.E.INVALID_PARAMS;              // invalid device
		if (device && !c.get("wireless", device))
			return ctx.util.E.INVALID_PARAMS;              // invalid device
		if (!is_opt_bool(random_bssid_p))
			return ctx.util.E.INVALID_PARAMS;              // invalid random_bssid
		if (!is_opt_int(param.channel, 0))
			return ctx.util.E.INVALID_PARAMS;              // invalid channel

		let need_restart = false;

		// ---- radio (wifi-device) branch ----
		if (device) {
			let band = c.get("wireless", device, "band") ?? "";
			let cfglist = this.get_config({}, ctx).res;
			let config = null;
			for (let cfg in cfglist)
				if (lc(cfg.band) == lc(band)) config = cfg;
			if (config == null)
				return ctx.util.E.INTERNAL_ERROR;          // get wifi config fail

			if (param.channel == 0) param.channel = "auto";

			set_htmode(c, device, param, band);

			set_opt(c, device, "require_mode", param.require_mode);
			if (band == "2g") set_opt(c, device, "legacy_rates", param.legacy_rates);

			let rt = radio_runtime(ctx);
			let ifname = (rt[device] && rt[device].ifname) ? rt[device].ifname : null;
			let htmode_check = c.get("wireless", device, "htmode") ?? "";
			if (param.channel != null && param.channel != "auto" &&
			    !channel_check(ctx, ifname, param.channel, band))
				return ctx.util.E.INVALID_PARAMS;          // invalid channel
			if (param.chan_6g_only_psc && param.channel != null && !channel_is_psc(param.channel))
				return ctx.util.E.INVALID_PARAMS;          // channel is not PSC while PSC is enabled

			// DEGRADE: repeater-mode channel-lock dropped (no gl repeater ubus)
			set_opt(c, device, "channel", param.channel);
			if (param.chan_6g_only_psc != null) set_opt(c, device, "chan_6g_only_psc", param.chan_6g_only_psc);

			// clamp htmode edge cases (ch 165 -> 20MHz)
			let mn = match(htmode_check, /([A-Za-z]+)([0-9]+)/);
			if (mn) {
				if (param.channel == 165 && band == "5g")
					c.set("wireless", device, "htmode", mn[1] + "20");
				else if (mn[1] == "HT" && int(mn[2]) > 40)
					c.set("wireless", device, "htmode", mn[1] + "40");
			}

			// txpower (only if a valid level was supplied; mirrors Lua's unconditional call)
			if (param.txpower == "Low" || param.txpower == "Medium" ||
			    param.txpower == "High" || param.txpower == "Max")
				apply_txpower(ctx, c, device, param.txpower, param.channel);
		}

		// ---- iface (wifi-iface) branch ----
		if (iface_name) {
			if (param.enabled != null) {
				param.disabled = !param.enabled;
				param.enabled = null;
				device = device ?? c.get("wireless", iface_name, "device");
			}
			let allowed_enc = { none: true, psk2: true, "psk-mixed": true, sae: true, "sae-mixed": true };
			if (param.encryption && !allowed_enc[param.encryption])
				return ctx.util.E.INVALID_PARAMS;          // invalid encryption
			// mac80211 uses these encryption names verbatim (no qca ccmp/sae translation)

			set_opt(c, iface_name, "disabled", param.disabled);
			set_opt(c, iface_name, "ssid", param.ssid);
			set_opt(c, iface_name, "hidden", param.hidden);
			set_opt(c, iface_name, "encryption", param.encryption);
			set_opt(c, iface_name, "key", param.key);
			set_opt(c, iface_name, "init", param.init);
			device = device ?? c.get("wireless", iface_name, "device");
			if (param.channel == "auto") {
				if (param.chan_6g_only_psc == false) c.set("wireless", iface_name, "acs_6g_only_psc", "0");
				else if (param.chan_6g_only_psc == true) c.set("wireless", iface_name, "acs_6g_only_psc", "1");
			}
			need_restart = set_random_bssid(c, ctx, iface_name, random_bssid_p);
		}

		// keep guest/iot network sections in sync with iface enablement
		setup_network_by_type(c, "guest");
		setup_network_by_type(c, "iot");

		c.commit("wireless");
		ctx.ubus.call("network", "reload", {});

		if (param.apply == false) return {};

		// DEGRADE: gl-cloud alive/send, gl_screen notify, guestwifi timer + reset-button dropped

		// bring the radio back up if hostapd isn't ENABLED (ported timer body, mac80211 only)
		if (device) {
			if (need_restart) {
				ctx.popen(`/sbin/wifi up ${device} >/dev/null 2>&1 &`);
			} else {
				let rt = radio_runtime(ctx);
				let ifname = (rt[device] && rt[device].ifname) ? rt[device].ifname : null;
				let state = phy_state(ctx, ifname);
				if (state && state != "ENABLED")
					ctx.popen(`/sbin/wifi up ${device} >/dev/null 2>&1 &`);
			}
		}
		return {};
	},

	set_txpower: function(args, ctx) {
		let param = args ?? {};
		let device = param.device;
		let level = param.txpower;
		if (device == null || !is_opt_str(device, 1, 256))
			return ctx.util.E.INVALID_PARAMS;              // invalid device
		let c = ctx.uci.cursor();
		if (!c.get("wireless", device))
			return ctx.util.E.INVALID_PARAMS;              // invalid device
		if (level != "Low" && level != "Medium" && level != "High" && level != "Max")
			return ctx.util.E.INVALID_PARAMS;              // invalid level
		apply_txpower(ctx, c, device, level, param.channel);
		return {};
	},

	// DEGRADE: Multi-Link Operation absent on SF1688 (Wi-Fi 6, no 6GHz/MLO hardware).
	// Return the shape the app expects with an empty iface set so the MLO tab renders inert.
	get_mlo_config: function(args, ctx) {
		return {
			res: {
				ifaces: [],
				encryptions: ["none", "psk2", "psk-mixed", "sae", "sae-mixed"],
				mlo_bandsupport: null,
				random_bssid: false
			}
		};
	},

	// DEGRADE: MLO unsupported -> reject config attempts rather than silently no-op.
	set_mlo_config: function(args, ctx) {
		return ctx.util.E.INVALID_PARAMS;                  // MLO not supported on this device
	}
};
