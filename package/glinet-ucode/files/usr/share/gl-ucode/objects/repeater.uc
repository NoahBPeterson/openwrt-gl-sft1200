// gl-ucode object: repeater  (port of decompiled-spec/rpc/repeater.lua — WiFi-as-WAN / STA mode)
// The GL `ubus repeater` daemon (STA join/scan/reconnect) is ABSENT on mainline, so the
// join/scan/status paths are rebuilt on wpa_supplicant STA + iwinfo/ubus + netifd:
//   scan       -> ubus iwinfo scan {device: <sta ifname>}
//   get_status -> iwinfo info + network.interface.wwan status (state semantics approximated)
//   connect    -> write a mode=sta wifi-iface into `wireless` + `network.wwan`, add to wan zone, reload
//   disconnect -> disable the sta wifi-iface + reload
//   saved-AP store -> our own `repeater` uci (config `network` sections), faithful to the original schema
// DEGRADE/DROP (see inline // DEGRADE): captive-portal AUTH enforcement (get/set_repeater_portal keep
//   the uci persistence + field names, but nothing actually authenticates to a portal); MAC clone/random
//   + disguise (validated for API compat, not applied — Siflower has no MAC-clone path); DFS/band-lock
//   readiness gate (qcawificfg80211/qcacld32 — MediaTek/QCA only; mac80211 needs no such gate);
//   enter/exit_bare_mode (sane stubs).
'use strict';

const fs = require('fs');

const STA_FALLBACK = "phy0-sta0";

function readfile(p) { let d = fs.readfile(p); return d ? trim(d) : null; }
function num(v) { return (v == null || v == "") ? null : int(v); }

// ---- validators (port of gl.validator.base + friends) ----
function valid_bool(v, opt) { return (v == null) ? (opt ? true : false) : (type(v) == "bool"); }
function valid_str(v, opt, lo, hi) {
	if (v == null) return opt ? true : false;
	if (type(v) != "string") return false;
	let L = length(v);
	return L >= lo && L <= hi;
}
function valid_int(v, opt, lo, hi) {
	if (v == null) return opt ? true : false;
	if (type(v) != "int" && type(v) != "double") return false;
	return v >= lo && v <= hi;
}
function valid_mac(mac) {
	if (type(mac) != "string") return false;
	return match(mac, /^[0-9a-fA-F]{2}(:[0-9a-fA-F]{2}){5}$/) ? true : false;
}
// GL is_valid_mac: well-formed AND a unicast address (low bit of first octet clear)
function is_valid_mac(mac) {
	if (!valid_mac(mac)) return false;
	let n = int(substr(mac, 0, 2), 16);   // ucode int() needs an explicit base for hex
	return (n % 2) == 0;
}
function valid_ip4(ip) {
	if (type(ip) != "string") return false;
	let m = match(ip, /^([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})$/);
	if (!m) return false;
	for (let i = 1; i <= 4; i++) if (int(m[i]) > 255) return false;
	return true;
}
function valid_netmask4(nm) { return valid_ip4(nm); }

// ---- wireless / sta helpers ----
function wireless_all(c) { return c.get_all("wireless") ?? {}; }

// name of the mode=sta wifi-iface section in `wireless`, or null
function find_sta_section(c) {
	let all = wireless_all(c);
	for (let k in all) {
		let s = all[k];
		if (s[".type"] == "wifi-iface" && s.mode == "sta") return s[".name"];
	}
	return null;
}

// first wifi-device radio name (for hosting the sta iface)
function first_radio(c) {
	let all = wireless_all(c);
	for (let k in all)
		if (all[k][".type"] == "wifi-device") return all[k][".name"];
	return null;
}

// live sta ifname from netifd wireless status (falls back to phy0-sta0)
function sta_ifname(ctx) {
	let dump = ctx.ubus.call("network.wireless", "status") ?? {};
	for (let radio in dump)
		for (let i in (dump[radio].interfaces ?? [])) {
			let cfg = i.config ?? {};
			if (cfg.mode == "sta" && i.ifname) return i.ifname;
		}
	return STA_FALLBACK;
}

function band_of(ch) {
	ch = int(ch);
	if (ch >= 1 && ch <= 14) return "2g";
	if (ch >= 32 && ch <= 196) return "5g";
	return "";
}

// one scannable ifname per radio (so scan covers BOTH 2.4 + 5 GHz, not just the
// band the STA currently sits on). Prefer any up iface on each radio.
function scan_devices(ctx) {
	let dump = ctx.ubus.call("network.wireless", "status") ?? {};
	let devs = [];
	for (let radio in dump) {
		if (!dump[radio].up) continue;
		for (let i in (dump[radio].interfaces ?? [])) {
			if (i.ifname) { push(devs, i.ifname); break; }
		}
	}
	if (length(devs) == 0) push(devs, STA_FALLBACK);
	return devs;
}

// wifi-device whose `band` matches "2g"/"5g" (radio0=2g, radio1=5g here); null if none
function radio_for_band(c, band) {
	if (band == null || band == "") return null;
	let all = wireless_all(c);
	for (let k in all)
		if (all[k][".type"] == "wifi-device" && all[k].band == band) return all[k][".name"];
	return null;
}

// Band arrangement for a single-radio-per-band device: put the client AP(s) on the
// band the uplink STA is NOT using. Result: uplink on one band, clients on the other,
// same SSID (seamless roam), no shared-channel/DFS-CAC conflict. Enables every mode=ap
// iface on a non-STA radio and disables those on the STA radio; leaves >=1 AP up.
function arrange_client_aps(c, sta_radio) {
	let all = wireless_all(c);
	let enabled_elsewhere = false;
	for (let k in all) {
		let s = all[k];
		if (s[".type"] != "wifi-iface" || s.mode != "ap") continue;
		if (s.device != sta_radio) { c.set("wireless", s[".name"], "disabled", "0"); enabled_elsewhere = true; }
	}
	// only take the STA's radio away from client APs if clients still have an AP
	// on the other band -- never strand every client.
	if (enabled_elsewhere)
		for (let k in all) {
			let s = all[k];
			if (s[".type"] == "wifi-iface" && s.mode == "ap" && s.device == sta_radio)
				c.set("wireless", s[".name"], "disabled", "1");
		}
}

// scan writes a bssid/ssid -> band cache here so connect can pick the right radio
// even though the app's connect params carry no band/channel.
const BAND_CACHE = "/tmp/gl-repeater-bands.json";

function write_band_cache(map) {
	let f = fs.open(BAND_CACHE, "w");
	if (f) { f.write(sprintf("%J", map)); f.close(); }
}

function cached_band(ssid, bssid) {
	let raw = fs.readfile(BAND_CACHE);
	if (!raw) return null;
	let m = json(raw);
	if (type(m) != "object") return null;
	if (bssid && m[uc(bssid)]) return m[uc(bssid)];
	if (ssid && m["ssid:" + ssid]) return m["ssid:" + ssid];
	return null;
}

// last resort: scan both radios for the target and return its band
function probe_band(ctx, ssid, bssid) {
	for (let dev in scan_devices(ctx)) {
		let sc = ctx.ubus.call("iwinfo", "scan", { device: dev }) ?? {};
		for (let r in (sc.results ?? [])) {
			if ((bssid && uc(r.bssid ?? "") == uc(bssid)) || (!bssid && r.ssid == ssid))
				return band_of(r.channel);
		}
	}
	return null;
}

// Append `netname` to the wan firewall zone's network list (idempotent).
function add_to_wan_zone(c, netname) {
	c.foreach("firewall", "zone", function(s) {
		if (s.name != "wan") return;
		let nets = s.network ?? [];
		if (type(nets) == "string") nets = split(trim(nets), /[ \t\n]+/);
		if (type(nets) != "array") nets = [];
		for (let n in nets) if (n == netname) return;
		push(nets, netname);
		c.set("firewall", s[".name"], "network", nets);
	});
}

// portal field layout by auth_mode (shared by scan + get_saved_ap_list)
function portal_fields(mode) {
	if (mode == 1) return ["one_click"];
	if (mode == 2) return ["password"];
	if (mode == 3) return ["username", "password"];
	if (mode == 4) return ["voucher"];
	return [];
}

// current repeater network section name = the saved AP matching the live STA ssid/bssid
function get_current_repeater_network(c, ctx) {
	let st = repeater_status(c, ctx);
	let cur = st.config;
	if (!cur || !cur.ssid) return null;
	let sid = null;
	c.foreach("repeater", "network", function(s) {
		if (s.ssid != cur.ssid) return;
		if (cur.bssid && cur.bssid != "" && s.bssid && s.bssid != "" && s.bssid != cur.bssid) return;
		sid = s[".name"];
		return false;
	});
	return sid;
}

// live status, shared by get_status + get_current_repeater_network
// state: -1 = no STA configured (initializing), 0 = disconnected, 1 = connecting/obtaining-ip, 2 = connected
function repeater_status(c, ctx) {
	let sta = find_sta_section(c);
	if (!sta) return { state: -1, config: {} };

	let dev = sta_ifname(ctx);
	let info = ctx.ubus.call("iwinfo", "info", { device: dev }) ?? {};
	let assoc = (info.ssid && info.bssid && info.bssid != "00:00:00:00:00:00");

	let config = {
		ssid: assoc ? info.ssid : (c.get("wireless", sta, "ssid") ?? ""),
		bssid: assoc ? info.bssid : (c.get("wireless", sta, "bssid") ?? "")
	};
	let res = { state: assoc ? 1 : 0, config: config };
	if (!assoc) return res;

	res.signal = info.signal;
	res.channel = info.channel;
	res.band = band_of(info.channel);

	// The STA's L3 interface is whatever `wireless.<sta>.network` points at — GL uses
	// "wwan", but our image binds it to "internet". Read it dynamically so state reaches
	// 2 (connected) once that interface has an IP; a hardcoded name leaves the app stuck
	// on "Connecting…" even though the uplink is up with internet.
	let stanet = c.get("wireless", sta, "network") ?? "wwan";
	let ws = ctx.ubus.call(`network.interface.${stanet}`, "status") ?? {};
	let v4 = ws["ipv4-address"];
	if (v4 && length(v4) > 0) {
		res.state = 2;
		res.ipaddr = v4[0].address;
		res.netmask = v4[0].mask;
		for (let r in (ws.route ?? [])) if (r.target == "0.0.0.0") res.gateway = r.nexthop;
		res.proto = ws.proto ?? "";
		res.dns = ws["dns-server"] ?? [];
	}
	return res;
}

export default {
	set_config: function(params, ctx) {
		params = params ?? {};
		let c = ctx.uci.cursor();
		let auto = params.auto;
		let smart_reconnect = params.smart_reconnect;
		let lock_band = params.lock_band;
		if (!valid_bool(auto, true)) return ctx.util.E.INVALID_PARAMS;
		if (smart_reconnect == null) smart_reconnect = true;
		if (!valid_bool(smart_reconnect, true)) return ctx.util.E.INVALID_PARAMS;
		if (lock_band != null && lock_band != "" && lock_band != "2g" && lock_band != "5g" && lock_band != "6g")
			return ctx.util.E.INVALID_PARAMS;
		c.set("repeater", "@main[0]", "auto", auto ? "1" : "0");
		c.set("repeater", "@main[0]", "smart_reconnect", smart_reconnect ? "1" : "0");
		c.set("repeater", "@main[0]", "lock_band", lock_band ?? "");
		c.commit("repeater");
		ctx.popen("sync");
		// DEGRADE: `ubus repeater reload` dropped — these are policy flags read by the
		// (absent) reconnect daemon; no live netifd effect, so no heavy reload here.
		return {};
	},

	get_config: function(params, ctx) {
		let c = ctx.uci.cursor();
		let res = {};
		c.foreach("repeater", "main", function(s) {
			res.auto = s.auto == "1";
			res.smart_reconnect = s.smart_reconnect == "1";
			res.lock_band = s.lock_band;
			res.macaddr = s.macaddr;
		});
		res.dfs_support = fs.access("/proc/gl-hw-info/dfs") ? true : false;
		return res;
	},

	get_channel_prompt: function(params, ctx) {
		return {
			chan_prompt_en: !fs.access("/tmp/chan_prompt_flag"),
			popup_prompt_en: !fs.access("/tmp/popup_prompt_flag")
		};
	},

	set_channel_prompt: function(params, ctx) {
		params = params ?? {};
		let chan_prompt_en = params.chan_prompt_en;
		let popup_prompt_en = params.popup_prompt_en;
		if (chan_prompt_en != null && !valid_bool(chan_prompt_en, false)) return ctx.util.E.INVALID_PARAMS;
		if (popup_prompt_en != null && !valid_bool(popup_prompt_en, false)) return ctx.util.E.INVALID_PARAMS;
		if (chan_prompt_en != null)
			ctx.popen(chan_prompt_en ? "rm -f /tmp/chan_prompt_flag" : "touch /tmp/chan_prompt_flag");
		if (popup_prompt_en != null)
			ctx.popen(popup_prompt_en ? "rm -f /tmp/popup_prompt_flag" : "touch /tmp/popup_prompt_flag");
		return {};
	},

	// DEGRADE: captive-portal AUTH is dropped (the daemon did the actual portal login).
	// These two methods keep the uci persistence + field names so the app round-trips config.
	get_repeater_portal: function(params, ctx) {
		let c = ctx.uci.cursor();
		let sid = get_current_repeater_network(c, ctx);
		if (!sid) return ctx.util.E.NOT_FOUND;
		let auth_mode_raw = c.get("repeater", sid, "portal_auth_mode");
		if (!auth_mode_raw || auth_mode_raw == "") return ctx.util.E.INVALID_PARAMS;
		let auth_mode = int(auth_mode_raw);
		if (!auth_mode || auth_mode < 1 || auth_mode > 4) return ctx.util.E.INVALID_PARAMS;
		let res = { auth_mode: auth_mode };
		if (auth_mode == 1) {
			let one_click = c.get("repeater", sid, "portal_one_click");
			if (one_click == "1") res.one_click = true;
			else if (one_click == "0") res.one_click = false;
		} else if (auth_mode == 2) {
			let password = c.get("repeater", sid, "portal_password");
			if (password && password != "") res.password = password;
		} else if (auth_mode == 3) {
			let username = c.get("repeater", sid, "portal_username");
			let password = c.get("repeater", sid, "portal_password");
			if (username && username != "") res.username = username;
			if (password && password != "") res.password = password;
		} else if (auth_mode == 4) {
			let voucher = c.get("repeater", sid, "portal_voucher");
			if (voucher && voucher != "") res.voucher = voucher;
		}
		return res;
	},

	set_repeater_portal: function(params, ctx) {
		params = params ?? {};
		let auth_mode = params.auth_mode;
		let save_config = params.save_config;
		let username, password, voucher, one_click;
		if (save_config == null) save_config = false;
		if (!valid_bool(save_config, false)) return ctx.util.E.INVALID_PARAMS;
		if (save_config) {
			if (!valid_int(auth_mode, false, 1, 4)) return ctx.util.E.INVALID_PARAMS;
			if (auth_mode == 1) {
				one_click = params.one_click;
				if (!valid_bool(one_click, false)) return ctx.util.E.INVALID_PARAMS;
			} else if (auth_mode == 2) {
				password = params.password;
				if (password == "") password = null;
				if (!valid_str(password, false, 1, 256)) return ctx.util.E.INVALID_PARAMS;
			} else if (auth_mode == 3) {
				username = params.username ?? params.usrname;
				password = params.password;
				if (username == "") username = null;
				if (password == "") password = null;
				if (!valid_str(username, false, 1, 256)) return ctx.util.E.INVALID_PARAMS;
				if (!valid_str(password, false, 1, 256)) return ctx.util.E.INVALID_PARAMS;
			} else if (auth_mode == 4) {
				voucher = params.voucher;
				if (voucher == "") voucher = null;
				if (!valid_str(voucher, false, 1, 1024)) return ctx.util.E.INVALID_PARAMS;
			}
		}
		let c = ctx.uci.cursor();
		let sid = get_current_repeater_network(c, ctx);
		if (!sid) return ctx.util.E.NOT_FOUND;
		c.delete("repeater", sid, "portal_auth_mode");
		c.delete("repeater", sid, "portal_one_click");
		c.delete("repeater", sid, "portal_username");
		c.delete("repeater", sid, "portal_password");
		c.delete("repeater", sid, "portal_voucher");
		if (save_config) {
			c.set("repeater", sid, "portal_auth_mode", `${auth_mode}`);
			if (auth_mode == 1) {
				if (one_click != null) c.set("repeater", sid, "portal_one_click", one_click ? "1" : "0");
			} else if (auth_mode == 2) {
				if (password != null) c.set("repeater", sid, "portal_password", password);
			} else if (auth_mode == 3) {
				if (username != null) c.set("repeater", sid, "portal_username", username);
				if (password != null) c.set("repeater", sid, "portal_password", password);
			} else if (auth_mode == 4 && voucher != null) {
				c.set("repeater", sid, "portal_voucher", voucher);
			}
		} else {
			// DEGRADE: gl_screen portal auth_status reset — GL-only config, applied only if present
			let auth_status = c.get("gl_screen", "portal", "auth_status");
			if (auth_status == "re_fail") {
				c.set("gl_screen", "portal", "auth_status", "waiting");
				c.commit("gl_screen");
			}
		}
		c.commit("repeater");
		ctx.popen("sync");
		return {};
	},

	// Join an AP as STA. Faithful validation port; the ubus-repeater `connect` is replaced by
	// writing a mode=sta wifi-iface + network.wwan and reloading netifd.
	connect: function(params, ctx) {
		params = params ?? {};
		let ssid = params.ssid;
		let identity = params.identity;
		let key = params.key;
		let remember = params.remember;
		let manual = params.manual;
		let network = params.network;
		let protocol = params.protocol;
		let ip = params.ip;
		let netmask = params.netmask;
		let gateway = params.gateway;
		let dns = params.dns;
		let mtu = params.mtu;
		let ttl = params.ttl;
		let ttl_ipv6 = params.ttl_ipv6;
		let auto_portal = params.auto_portal;
		let bssid = params.bssid;
		let disguise = params.disguise;

		if (!valid_bool(disguise, true)) return ctx.util.E.INVALID_PARAMS;
		if (!valid_str(ssid, false, 1, 32)) return ctx.util.E.INVALID_PARAMS;
		if (bssid != null && !is_valid_mac(bssid)) return ctx.util.E.INVALID_PARAMS;
		if (!valid_str(identity, true, 1, 256)) return ctx.util.E.INVALID_PARAMS;
		if (!valid_str(key, true, 1, 256)) return ctx.util.E.INVALID_PARAMS;
		if (!valid_bool(remember, true)) return ctx.util.E.INVALID_PARAMS;
		if (!valid_bool(manual, true)) return ctx.util.E.INVALID_PARAMS;
		if (!valid_bool(auto_portal, true)) return ctx.util.E.INVALID_PARAMS;
		if (!valid_str(network, true, 1, 256)) return ctx.util.E.INVALID_PARAMS;
		if (!valid_int(mtu, true, 576, 9200)) return ctx.util.E.INVALID_PARAMS;
		if (!valid_int(ttl, true, 1, 255)) return ctx.util.E.INVALID_PARAMS;
		if (!valid_int(ttl_ipv6, true, 1, 255)) return ctx.util.E.INVALID_PARAMS;
		if (protocol != null) {
			if (protocol != "dhcp" && protocol != "static") return ctx.util.E.INVALID_PARAMS;
			if (protocol == "static") {
				if (!valid_ip4(ip)) return ctx.util.E.INVALID_PARAMS;
				if (!valid_netmask4(netmask)) return ctx.util.E.INVALID_PARAMS;
				if (!valid_ip4(gateway)) return ctx.util.E.INVALID_PARAMS;
				if (dns != null) {
					if (type(dns) != "array") return ctx.util.E.INVALID_PARAMS;
					for (let d in dns) if (!valid_ip4(d)) return ctx.util.E.INVALID_PARAMS;
				}
			}
		}
		// macaddr: the app sends a STRING "<mode>,<mac>" (c=clone, r*=random+update) -- the
		// same shape get_saved_ap_list stores/parses; older callers may send an object.
		// Effect is DROPPED on Siflower (no MAC clone/random path), so validate loosely and
		// never reject a real connect over its shape (this was the "invalid params" bug).
		if (!disguise && params.macaddr != null) {
			let ma = params.macaddr;
			if (type(ma) == "string") {
				// "<mode>,<mac>" -- check FORMAT only (valid_mac), not unicast: the app's
				// random MAC can have an odd/multicast first octet, and the effect is dropped.
				let m = match(ma, /^([0-9a-zA-Z]+),(.+)$/);
				if (m && !valid_mac(trim(m[2]))) return ctx.util.E.INVALID_PARAMS;
			} else if (type(ma) == "object") {
				let mode = ma.mode;
				if (mode != null && mode != "default" && mode != "clone" && mode != "random") return ctx.util.E.INVALID_PARAMS;
				if (mode != null && mode != "default" && ma.macaddr != null && !valid_mac(ma.macaddr)) return ctx.util.E.INVALID_PARAMS;
			} else {
				return ctx.util.E.INVALID_PARAMS;
			}
		}
		// DEGRADE: disguise (clone client MAC onto the STA) is dropped; treated as default.

		let c = ctx.uci.cursor();
		let netname = network ?? "wwan";
		let enc = (key != null && key != "") ? "psk2" : "none";
		// DEGRADE: encryption simplified to psk2/none; EAP(identity)/SAE-only negotiated by wpa_supplicant.

		// live sta wifi-iface
		let sta = find_sta_section(c) ?? c.add("wireless", "wifi-iface");
		// Host the STA on the radio whose band matches the target AP, else a 5 GHz AP
		// can never associate (a 2.4-only radio can't see it). Band comes from the scan
		// cache (normal scan->connect flow), falling back to a live probe, then the
		// existing/first radio.
		let tband = cached_band(ssid, bssid) ?? probe_band(ctx, ssid, bssid);
		let radio = radio_for_band(c, tband) ?? c.get("wireless", sta, "device") ?? first_radio(c);
		if (radio) {
			c.set("wireless", sta, "device", radio);
			// keep the client AP on the other band so devices stay online (and no AP
			// ever lands on a DFS uplink channel -> no 60s radar check).
			arrange_client_aps(c, radio);
		}
		c.set("wireless", sta, "mode", "sta");
		c.set("wireless", sta, "network", netname);
		c.set("wireless", sta, "ssid", ssid);
		if (bssid) c.set("wireless", sta, "bssid", bssid); else c.delete("wireless", sta, "bssid");
		c.set("wireless", sta, "encryption", enc);
		if (enc != "none") c.set("wireless", sta, "key", key); else c.delete("wireless", sta, "key");
		c.set("wireless", sta, "disabled", "0");
		c.commit("wireless");

		// wwan network iface
		c.set("network", netname, "interface");
		c.set("network", netname, "proto", protocol ?? "dhcp");
		if (protocol == "static") {
			c.set("network", netname, "ipaddr", ip);
			c.set("network", netname, "netmask", netmask);
			c.set("network", netname, "gateway", gateway);
			if (dns != null) c.set("network", netname, "dns", dns);
		} else {
			c.delete("network", netname, "ipaddr");
			c.delete("network", netname, "netmask");
			c.delete("network", netname, "gateway");
			c.delete("network", netname, "dns");
		}
		if (mtu != null) c.set("network", netname, "mtu", `${mtu}`); else c.delete("network", netname, "mtu");
		add_to_wan_zone(c, netname);
		c.commit("network");
		c.commit("firewall");

		// remember: persist to our saved-AP store (config `network` sections of `repeater`)
		if (remember) {
			let existing = null;
			c.foreach("repeater", "network", function(s) {
				if (s.ssid == ssid) { existing = s[".name"]; return false; }
			});
			let nid = existing ?? c.add("repeater", "network");
			c.set("repeater", nid, "ssid", ssid);
			if (bssid) c.set("repeater", nid, "bssid", bssid);
			if (key != null && key != "") c.set("repeater", nid, "key", key);
			if (identity != null && identity != "") c.set("repeater", nid, "identity", identity);
			c.set("repeater", nid, "proto", protocol ?? "dhcp");
			if (protocol == "static") {
				c.set("repeater", nid, "ipaddr", ip);
				c.set("repeater", nid, "netmask", netmask);
				c.set("repeater", nid, "gateway", gateway);
				if (dns != null) c.set("repeater", nid, "dns", dns);
			}
			if (mtu != null) c.set("repeater", nid, "mtu", `${mtu}`);
			if (ttl != null) c.set("repeater", nid, "ttl", `${ttl}`);
			if (ttl_ipv6 != null) c.set("repeater", nid, "hl", `${ttl_ipv6}`);
			c.set("repeater", nid, "auto_portal", auto_portal ? "1" : "0");
			c.set("repeater", nid, "disguise", disguise ? "1" : "0");
			// mark as the selected/manual entry (`scan` flag == the manually-chosen AP)
			c.foreach("repeater", "network", function(s) {
				c.set("repeater", s[".name"], "selected", s[".name"] == nid ? "1" : "0");
				if (s[".name"] != nid) c.delete("repeater", s[".name"], "scan");
			});
			if (manual) c.set("repeater", nid, "scan", "1"); else c.delete("repeater", nid, "scan");
			c.commit("repeater");
		}

		ctx.popen("sync");
		ctx.ubus.call("network", "reload");
		// Detach with setsid so the reconfigure survives the RPC handler exiting -- a
		// plain `&` gets SIGHUP'd mid-reload and the client-AP re-arrangement never
		// completes (leaves clients with no AP after a band swap).
		ctx.popen("setsid /sbin/wifi reload </dev/null >/dev/null 2>&1 &");
		return {};
	},

	get_saved_ap_list: function(params, ctx) {
		let c = ctx.uci.cursor();
		let res = [];
		c.foreach("repeater", "network", function(s) {
			let entry = {
				ssid: s.ssid,
				bssid: s.bssid,
				key: s.key,
				identity: s.identity,
				protocol: s.proto,
				ip: s.ipaddr,
				netmask: s.netmask,
				gateway: s.gateway,
				dns: s.dns,
				mtu: num(s.mtu),
				ttl: num(s.ttl),
				ttl_ipv6: num(s.hl),
				manual: s.scan == "1",
				auto_portal: s.auto_portal == "1",
				disguise: s.disguise == "1"
			};
			if (!entry.disguise) {
				let macaddr = s.macaddr;
				let mode = "default";
				let update = "none";
				let period;
				if (macaddr) {
					let m = match(macaddr, /^([0-9a-zA-Z]+),(.+)$/);
					if (m) {
						let prefix = m[1];
						macaddr = m[2];
						if (prefix == "c") {
							mode = "clone";
						} else {
							mode = "random";
							prefix = substr(prefix, 1);
							if (prefix == "r") update = "reboot";
							else if (length(prefix) > 0) { update = "time"; period = int(prefix) / 60; }
						}
					} else {
						macaddr = null;
					}
				}
				entry.macaddr = { mode: mode, macaddr: macaddr, update: update, period: period };
			}
			let pmode = num(s.portal_auth_mode) ?? 0;
			if (pmode > 0) {
				entry.portal_info = { auth_mode: pmode };
				for (let field in portal_fields(pmode)) {
					let value = s["portal_" + field];
					if (value && value != "") entry.portal_info[field] = value;
				}
			}
			push(res, entry);
		});
		return { res: res };
	},

	remove_saved_ap: function(params, ctx) {
		params = params ?? {};
		let c = ctx.uci.cursor();
		let ssid = params.ssid;
		let deleted_had_scan = false;
		let selected_sid = null;
		let remain_sids = [];
		if (!valid_str(ssid, false, 1, 32)) return ctx.util.E.INVALID_PARAMS;
		c.foreach("repeater", "network", function(s) {
			if (s.ssid == ssid) {
				if (s.scan) deleted_had_scan = true;
				c.delete("repeater", s[".name"]);
			} else {
				let sid = s[".name"];
				push(remain_sids, sid);
				if (s.selected == "1") selected_sid = sid;
			}
		});
		if (deleted_had_scan) {
			let target = selected_sid ?? remain_sids[0];
			if (target) {
				for (let sid in remain_sids) {
					if (sid == target) c.set("repeater", sid, "scan", "1");
					else c.delete("repeater", sid, "scan");
				}
			}
		}
		c.commit("repeater");
		ctx.popen("sync");
		// DEGRADE: `ubus repeater reload` dropped; disconnect/connect drive netifd directly.
		return {};
	},

	disconnect: function(params, ctx) {
		let c = ctx.uci.cursor();
		let sta = find_sta_section(c);
		if (sta) c.set("wireless", sta, "disabled", "1");
		// STA off -> no uplink band to avoid, so restore every client AP (both bands)
		// or a prior 5 GHz band-swap could leave clients stranded on one band.
		let all = wireless_all(c);
		for (let k in all) {
			let s = all[k];
			if (s[".type"] == "wifi-iface" && s.mode == "ap")
				c.set("wireless", s[".name"], "disabled", "0");
		}
		c.commit("wireless");
		ctx.popen("sync");
		ctx.ubus.call("network", "reload");
		// detach so the reconfigure survives the handler exit (a plain & is SIGHUP'd)
		ctx.popen("setsid /sbin/wifi reload </dev/null >/dev/null 2>&1 &");
		return {};
	},

	get_status: function(params, ctx) {
		let c = ctx.uci.cursor();
		return repeater_status(c, ctx);
	},

	// Scan visible APs via iwinfo, merge with the saved-AP store. The GL DFS/band-lock readiness
	// gate (qcawificfg80211/qcacld32) is DROPPED — mac80211 needs no such gate.
	scan: function(params, ctx) {
		params = params ?? {};
		if (!valid_bool(params.refresh, true)) return ctx.util.E.INVALID_PARAMS;
		let c = ctx.uci.cursor();

		// saved-network lookup (ssid -> portal info) for the `saved`/`portal_info` fields
		let saved_networks = {};
		c.foreach("repeater", "network", function(s) {
			saved_networks[s.ssid] = {
				portal_auth_mode: num(s.portal_auth_mode) ?? 0,
				portal_password: s.portal_password ?? "",
				portal_username: s.portal_username ?? "",
				portal_voucher: s.portal_voucher ?? "",
				portal_one_click: s.portal_one_click == "1"
			};
		});

		// scan EVERY radio (2.4 + 5 GHz), not just the STA's current band, and merge.
		let devs = scan_devices(ctx);
		let results = [], seen_bssid = {};
		for (let dev in devs) {
			let scan = ctx.ubus.call("iwinfo", "scan", { device: dev }) ?? {};
			for (let r in (scan.results ?? [])) {
				let bk = uc(r.bssid ?? "");
				if (bk != "" && seen_bssid[bk]) continue;   // de-dup APs seen on both
				if (bk != "") seen_bssid[bk] = true;
				r._dev = dev;
				push(results, r);
			}
		}

		let band_cache = {};
		let survey = [];
		for (let s in results) {
			let ssid = s.ssid;
			if (!ssid || ssid == "") continue;           // hidden / control-only SSID

			let enc = s.encryption ?? {};
			// WEP-only networks are unsupported (mirror original PRIVACY && !rsn && !wpa skip)
			if (enc.enabled && enc.wep && (!enc.wpa || length(enc.wpa) == 0)) continue;

			let suites = [];
			let owe = false;
			for (let a in (enc.authentication ?? [])) {
				let t = uc(a);
				if (t == "OWE") owe = true;
				push(suites, t);
			}
			// Faithful to repeater.lua: encryption is exactly {enabled, description}.
			let encryption = {
				enabled: enc.enabled ? (owe ? false : true) : false,
				description: join(",", suites)
			};

			// iwinfo's `band` is an int index; the app parses `band` as a String? and
			// throws on an int (dropping the whole scan item). Use band_of() -> "2g"/"5g".
			let band = band_of(s.channel);
			if (s.bssid) band_cache[uc(s.bssid)] = band;
			band_cache["ssid:" + ssid] = band;
			let saved_net = saved_networks[ssid];
			let result = {
				ssid: ssid,
				encryption: encryption,
				bssid: s.bssid,
				signal: s.signal,                    // raw dBm int, faithful to info.signal
				channel: s.channel,
				dfs: s.dfs,
				band: band,
				device: s._dev,
				saved: saved_net != null
			};
			if (saved_net && saved_net.portal_auth_mode > 0) {
				result.portal_info = { auth_mode: saved_net.portal_auth_mode };
				for (let field in portal_fields(saved_net.portal_auth_mode)) {
					let value = saved_net["portal_" + field];
					if (value && value != "") result.portal_info[field] = value;
				}
			}
			push(survey, result);
		}
		write_band_cache(band_cache);
		return { res: survey };
	},

	// DEGRADE: bare mode (temporary open AP for onboarding) needs the GL daemon — return a stub.
	enter_bare_mode: function(params, ctx) {
		return {};
	},

	exit_bare_mode: function(params, ctx) {
		return {};
	}
};
