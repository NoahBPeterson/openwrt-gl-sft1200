// gl-ucode object: vpn-client  (port of decompiled-spec/rpc/vpn-client.lua)
// The split-tunnel policy-routing engine behind the VPN dashboard. All config lives
// in the GL-only `route_policy` uci (global/default/rule sections with tunnel_id,
// via_type, from/to ipset refs, killswitch) -- KEPT as our own store so config
// round-trips exactly. The tunnels themselves are stock wireguard/openvpn (uci
// `wireguard`/`ovpnclient` + `network`).
//
// DEGRADE (apply layer): GL's /etc/init.d/vpn-client + tunnel-switch.sh + the
// domain-list refresh cron (gl_timer / update_vpn_domain.sh) are ABSENT on mainline.
// Policy application is reimplemented natively in apply_policy(): src-MAC nft sets ->
// fwmark -> per-tunnel `ip route` table + `ip rule` policy routing, with killswitch
// blackholing, for the common from.type=="mac" case with a connected VPN iface.
// Other selectors (interface zones, domain dst_net ipsets, exclude_* variants,
// novpn/autovpn) need the GL domain resolver / zone map and are STORE-ONLY (the uci
// still round-trips; only enforcement is skipped). See per-site `// DEGRADE:` notes.
// The tap-s2s / mptun conflict guards are DROPPED per port scope (set_tap_s2s is
// kept as a degraded store-only stub). Cross-module gl-session fan-out in
// get_all_config_list is attempted then falls back to reading uci directly.
// gl.vpn_err_code is absent -> generic {err_code,err_msg} objects (field names kept).
// PRIORITY: get_status is solid + non-crashing and returns an empty list when there
// are no tunnels.
'use strict';

const fs = require('fs');

const PROFILE_BASE   = "/etc/vpn_profiles.d/";
const DOMAIN_PATH    = "/etc/domain_mac_list/";
const TMP_DOMAINLIST = "/tmp/tmp_vpn_domainlist.txt";

// tunnel connection status
const STATUS_NOT_START = 0;
const STATUS_CONNECTED = 1;
const STATUS_CONNECTING = 2;
// per-client vpn-using status
const STATUS_NOT_USE = 0;
const STATUS_USE_VPN = 1;
const STATUS_VPN_DOWN = 2;
const STATUS_NOT_EFFORT = 3;
const STATUS_VPN_DOWN_AND_WAN_BLOCK = 4;

// native apply bands (fully owned + cleaned each apply, so non-destructive)
const TABLE_BASE = 41000;
const PREF_BASE  = 30000;
const MAX_T      = 32;

const MAC_RE = /^[0-9a-fA-F]{2}(:[0-9a-fA-F]{2}){5}$/;

// ---- error objects (gl.vpn_err_code absent; field names preserved) -----------
function e_missing() { return { err_code: -10, err_msg: "parameter missing" }; }
function e_invalid() { return { err_code: -1, err_msg: "invalid parameter" }; }

// ---- small utilities ---------------------------------------------------------
function safe_ifname(x) {
	return (type(x) == "string" && match(x, /^[A-Za-z0-9_.-]+$/)) ? x : null;
}

function split_host_port(s) {
	if (type(s) != "string") return [null, null];
	let m = match(s, /^(.*):([0-9]+)$/);
	if (m) return [m[1], m[2]];
	return [s, null];
}

function ensure_section(c, section, type_name) {
	if (c.get("route_policy", section) == null)
		c.set("route_policy", section, type_name);
}

function reorder_section(c, section, idx) {
	// ucode uci exposes reorder; guard so a build without it degrades to no reorder.
	if (type(c.reorder) == "function") {
		try { c.reorder("route_policy", section, idx); } catch (e) {}
	}
	// DEGRADE: if reorder is unavailable, section order is left as-is (config still saved).
}

function tool_available(ctx, name) {
	return ctx.popen(`command -v ${name} 2>/dev/null`) ? true : false;
}

// ---- tunnel id / index helpers ----------------------------------------------
function get_tunnel_sid(c, tunnel) {
	let found = { sid: null, index: null };
	let scan = function(s) {
		let tid = (s.tunnel_id != null) ? int(s.tunnel_id) : null;
		if (tid == tunnel) { found.sid = s[".name"]; found.index = s[".index"]; }
	};
	c.foreach("route_policy", "default", scan);
	c.foreach("route_policy", "rule", scan);
	return found;
}

function generate_tunnel_id(c) {
	// rand()/srand() aren't globals in the uhttpd ucode VM; walk 1..9999 from a
	// time-derived seed so we still return an unused id (deterministic, no PRNG).
	let seed = (time() % 9999) + 1;
	let n = 9999;
	while (n > 0) {
		if (!get_tunnel_sid(c, seed).sid) return seed;
		seed = (seed % 9999) + 1;
		n--;
	}
	return 0;
}

// ---- vpn client existence index (wireguard peers / ovpnclient clients) -------
function build_vpn_client_index(c) {
	let index = { wireguard: {}, openvpn: {} };
	c.foreach("wireguard", "peers", function(s) {
		let mm = match(s[".name"], /[0-9]+/);
		let peer_id = (s.peer_id != null) ? int(s.peer_id) : (mm ? int(mm[0]) : null);
		let group_id = (s.group_id != null) ? int(s.group_id) : null;
		if (group_id != null && peer_id != null) {
			index.wireguard[group_id] ??= {};
			index.wireguard[group_id][peer_id] = true;
		}
	});
	c.foreach("ovpnclient", "clients", function(s) {
		let client_id = (s.client_id != null) ? int(s.client_id) : null;
		let group_id = (s.group_id != null) ? int(s.group_id) : null;
		if (group_id != null && client_id != null) {
			index.openvpn[group_id] ??= {};
			index.openvpn[group_id][client_id] = true;
		}
	});
	return index;
}

function vpn_client_exists(index, via_type, group_id, client_id) {
	let group_map = (via_type == "wireguard") ? index.wireguard[group_id] : index.openvpn[group_id];
	return (group_map && group_map[client_id]) ? true : false;
}

function build_candidates(via, via_type) {
	let kind = (via_type == "wireguard") ? "peer_id" : "client_id";
	let entries = [];
	if (type(via) == "object" && type(via.configs) == "array") {
		for (let cfg in via.configs) {
			let gid_num = (cfg.group_id != null) ? int(cfg.group_id) : null;
			if (gid_num == null || gid_num < 1) return { err: e_invalid() };
			let raw = cfg.id_list;
			if (type(raw) != "array") return { err: e_invalid() };
			for (let v in raw) {
				let id_num = (v != null) ? int(v) : null;
				if (id_num == null || id_num < 1) return { err: e_invalid() };
				push(entries, { group_id: gid_num, id: id_num });
			}
		}
	}
	let first = entries[0];
	if (!first || first.id == null) return { err: e_invalid() };
	return { cands: { kind: kind, entries: entries, current: first.id, current_group_id: first.group_id } };
}

// ---- profile file (candidate list persisted at /etc/vpn_profiles.d/profileN) -
function profile_build_path(tunnel_id, existing) {
	if (type(existing) == "string" && existing != "") {
		if (substr(existing, 0, length(PROFILE_BASE)) != PROFILE_BASE) return null;
		return existing;
	}
	if (tunnel_id == null) return null;
	return PROFILE_BASE + "profile" + tunnel_id;
}

function profile_parse_line(line) {
	let t = trim(line);
	if (t == "" || substr(t, 0, 1) == "#") return null;
	let m = match(t, /^([0-9]+)_([0-9]+)$/);
	if (m) return { group_id: int(m[1]), id: int(m[2]) };
	return null;
}

function profile_read_lines(path) {
	let result = [];
	if (type(path) != "string" || path == "" || !fs.access(path)) return result;
	let content = fs.readfile(path) ?? "";
	if (content == "") return result;
	for (let line in split(content, "\n")) {
		if (line == "") continue;
		let p = profile_parse_line(line);
		push(result, { text: line, group_id: p ? p.group_id : null, id: p ? p.id : null });
	}
	return result;
}

function profile_read_grouped(path) {
	let result = [], gid_index = {};
	for (let ln in profile_read_lines(path)) {
		if (ln.group_id != null) {
			let key = "" + ln.group_id;
			if (gid_index[key] == null) {
				push(result, { group_id: ln.group_id, id_list: [ ln.id ] });
				gid_index[key] = length(result) - 1;
			} else {
				push(result[gid_index[key]].id_list, ln.id);
			}
		}
	}
	return result;
}

function profile_write(ctx, path, entries) {
	if (type(path) != "string" || path == "") return;
	let m = match(path, /^(.*)\/[^\/]+$/);
	let dir = m ? m[1] : null;
	if (dir && dir != "") ctx.popen("mkdir -p " + dir);
	let lines = [], seen = {};
	for (let entry in (entries ?? [])) {
		let gid = entry.group_id, id = entry.id;
		if (gid != null && id != null && id != "") {
			let line = gid + "_" + id;
			if (!seen[line]) { seen[line] = true; push(lines, line); }
		}
	}
	if (length(lines) == 0) { fs.writefile(path, ""); return; }
	fs.writefile(path, join("\n", lines) + "\n");
}

function profile_filter(ctx, path, check_fn) {
	let lines = profile_read_lines(path);
	let kept = [], valid = [], any_removed = false;
	for (let ln in lines) {
		if (ln.group_id != null) {
			if (check_fn(ln.group_id, ln.id)) {
				push(kept, ln.text);
				push(valid, { group_id: ln.group_id, id: ln.id });
			} else {
				any_removed = true;
			}
		} else {
			push(kept, ln.text);
		}
	}
	if (any_removed) fs.writefile(path, join("\n", kept));
	return { valid: valid, changed: any_removed };
}

function profile_remove(tunnel_id) {
	fs.unlink(PROFILE_BASE + "profile" + tunnel_id);
}

// ---- domain list download / validation (for check_domain_online + url policy)
function is_ip6(s) {
	return (type(s) == "string" && index(s, ":") >= 0 && match(s, /^[0-9A-Fa-f:]+$/)) ? true : false;
}

function is_valid_line(line) {
	if (line == "") return false;
	let fc = substr(line, 0, 1);
	if (fc == "#") return false;
	if (fc >= "0" && fc <= "9") {
		if (match(line, /^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$/) || match(line, /^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+\/[0-9]+$/))
			return true;
		else if (index(line, ":") >= 0 && is_ip6(line))
			return true;
		return false;
	}
	if (index(line, ":") >= 0) return is_ip6(line);
	// dash placed last + unescaped in every class (ucode regex rejects escaped dash)
	if (index(line, ".") >= 0 && !match(line, /^[.-]/) && !match(line, /[.-]$/) && index(line, "..") < 0 &&
	    match(line, /^[A-Za-z0-9_.-]+$/) && match(line, /^[A-Za-z0-9][A-Za-z0-9_.-]*\.[A-Za-z][A-Za-z0-9_-]+$/))
		return true;
	return false;
}

function download_domain_list(ctx, url) {
	let res = { success: 0, total: 0, failed: 0, invalid_lines: [] };
	if (type(url) != "string" || url == "") return res;
	if (!tool_available(ctx, "curl")) return res;   // DEGRADE: no curl -> nothing fetched

	// write the url into a curl -K config file (mirrors the original) so it never
	// reaches the shell command line -> no command injection via the url.
	let cfg = "/tmp/vpn_dl_url.conf";
	let esc = replace(replace(url, "\\", "\\\\"), "\"", "\\\"");
	fs.writefile(cfg, `url = "${esc}"\n`);
	let raw = ctx.popen(`curl -k -L -sf --connect-timeout 20 -m 20 -K ${cfg} 2>/dev/null`);
	fs.unlink(cfg);
	if (raw == null) raw = "";

	// light binary guard (original rejected content with control bytes up front)
	let ctrl = 0, n = length(raw), lim = (n < 16) ? n : 16;
	for (let i = 0; i < lim; i++) {
		let b = ord(raw, i);
		if (b == 0 || (b < 32 && b != 9 && b != 10 && b != 13)) ctrl++;
	}
	if (ctrl > 2) return res;

	let valid = [], ln = 0;
	for (let line in split(raw, "\n")) {
		if (ln >= 200000) break;
		ln++;
		let t = trim(line);
		if (t == "" || substr(t, 0, 1) == "#") continue;
		if (is_valid_line(t)) { push(valid, t); res.success++; }
		else { res.failed++; if (length(res.invalid_lines) < 10) push(res.invalid_lines, ln); }
	}
	res.total = ln;
	if (length(valid) > 0) fs.writefile(TMP_DOMAINLIST, join("\n", valid) + "\n");
	else fs.unlink(TMP_DOMAINLIST);
	return res;
}

function apply_url_policy(ctx, tunnel_id, url) {
	let res = download_domain_list(ctx, url);
	let dfile = DOMAIN_PATH + "dst_net" + tunnel_id;
	ctx.popen("rm -f " + dfile);
	if (res && res.success != 0 && fs.access(TMP_DOMAINLIST)) {
		ctx.popen("mkdir -p " + DOMAIN_PATH);
		ctx.popen("mv " + TMP_DOMAINLIST + " " + dfile);
	}
	// DEGRADE: GL's periodic refresh cron (gl_timer + /usr/bin/update_vpn_domain.sh)
	// is absent -> the list is fetched once here but not auto-refreshed.
}

// ---- options ----------------------------------------------------------------
function save_options(ctx, params, c, sid) {
	let need_commit = false;
	if (!c) { c = ctx.uci.cursor(); need_commit = true; }
	if (!sid) { let f = get_tunnel_sid(c, params.tunnel_id); sid = f.sid; }
	if (!sid) return e_invalid();

	if ((type(params.mtu) == "int" || type(params.mtu) == "double") && params.mtu >= 68 && params.mtu <= 65535) {
		c.set("route_policy", sid, "mtu", "" + int(params.mtu));
	} else {
		if (params.mtu != null && params.mtu != 0) return e_invalid();
		c.delete("route_policy", sid, "mtu");
	}
	if (params.masq == false) c.set("route_policy", sid, "masq", "0");
	else c.delete("route_policy", sid, "masq");
	if (params.local_access == true) c.set("route_policy", sid, "local_access", "1");
	else c.delete("route_policy", sid, "local_access");
	if (params.service_policy == true) c.set("route_policy", sid, "service_policy", "1");
	else c.delete("route_policy", sid, "service_policy");
	if (params.killswitch != null) c.set("route_policy", sid, "killswitch", params.killswitch ? "1" : "0");

	if (need_commit) { c.commit("route_policy"); system("sync 2>/dev/null"); }
	return {};
}

// ---- core: write a rule section from a params/config object ------------------
function add_tunnel_config(c, params, ctx) {
	let sid = c.add("route_policy", "rule");
	if (type(params.name) == "string") c.set("route_policy", sid, "name", params.name);
	let tunnel_id = params.tunnel_id;
	c.set("route_policy", sid, "tunnel_id", "" + tunnel_id);
	c.set("route_policy", sid, "enabled", params.enabled ? "1" : "0");
	if (params.killswitch != null) c.set("route_policy", sid, "killswitch", params.killswitch ? "1" : "0");

	let via = params.via ?? {};
	if (params.enabled && !via.type) { c.delete("route_policy", sid); return { ok: false, err: e_invalid() }; }

	if (via.type == "novpn") {
		c.set("route_policy", sid, "via_type", "novpn");
	} else if (via.type == "wireguard" || via.type == "openvpn") {
		if (type(via.configs) != "array") { c.delete("route_policy", sid); return { ok: false, err: e_missing() }; }
		let bc = build_candidates(via, via.type);
		if (bc.err) { c.delete("route_policy", sid); return { ok: false, err: bc.err }; }
		let candidates = bc.cands;
		let vpn_index = build_vpn_client_index(c);
		let valid_entries = [];
		for (let entry in candidates.entries)
			if (vpn_client_exists(vpn_index, via.type, entry.group_id, entry.id)) push(valid_entries, entry);
		if (length(valid_entries) == 0) {
			c.delete("route_policy", sid);
			c.commit("route_policy");
			return { ok: false, err: e_invalid() };
		}
		candidates.entries = valid_entries;
		candidates.current = valid_entries[0].id;
		candidates.current_group_id = valid_entries[0].group_id;
		let current_group_id = candidates.current_group_id ?? via.group_id;
		if (current_group_id == null || current_group_id == "") { c.delete("route_policy", sid); return { ok: false, err: e_invalid() }; }

		c.set("route_policy", sid, "via_type", via.type);
		c.set("route_policy", sid, "group_id", "" + current_group_id);
		if (candidates.kind == "peer_id") {
			c.set("route_policy", sid, "peer_id", "" + candidates.current);
			c.delete("route_policy", sid, "client_id");
		} else {
			c.set("route_policy", sid, "client_id", "" + candidates.current);
			c.delete("route_policy", sid, "peer_id");
		}

		let profiles_path = null;
		let write_profiles = params._write_profiles == true;
		if (write_profiles || (type(params.profiles) == "string" && params.profiles != "")) {
			profiles_path = profile_build_path(tunnel_id, params.profiles);
			if (!profiles_path) { c.delete("route_policy", sid); return { ok: false, err: e_invalid() }; }
			c.set("route_policy", sid, "profiles", profiles_path);
		}
		if (write_profiles && profiles_path) profile_write(ctx, profiles_path, candidates.entries);
	} else if (via.type == "autovpn") {
		// DEGRADE: setup_autovpn_instance.lua / eco spawn absent -> stored but not spawned.
		c.set("route_policy", sid, "via_type", "autovpn");
	}

	let from = params.from ?? {};
	if (from.type == "interface" && from.interface_list && length(from.interface_list) != 0) {
		c.set("route_policy", sid, "from_type", "interface");
		c.set("route_policy", sid, "from", from.interface_list);
	} else if ((from.type == "mac" || from.type == "exclude_mac") && from.mac_list && length(from.mac_list) != 0) {
		let prefix = (from.type == "mac") ? "src_mac" : "!src_mac";
		c.set("route_policy", sid, "from_type", "ipset");
		c.set("route_policy", sid, "from", prefix + tunnel_id);
		c.set("route_policy", sid, "from_mac", from.mac_list);
	} else if (from.type != "default") {
		c.delete("route_policy", sid);
		return { ok: false, err: e_missing() };
	}

	let to = params.to ?? {};
	if (to.type == "domain" || to.type == "exclude_domain") {
		let prefix = (to.type == "domain") ? "dst_net" : "!dst_net";
		c.set("route_policy", sid, "to_type", "ipset");
		if (to.manual && to.domain_list && length(to.domain_list) != 0) {
			c.set("route_policy", sid, "to_manual", "1");
			c.set("route_policy", sid, "to", prefix + tunnel_id);
			c.set("route_policy", sid, "to_list_external", "/etc/domain_mac_list/dst_net" + tunnel_id);
			if (to.url != null) c.set("route_policy", sid, "to_url", to.url);
			let base_path = DOMAIN_PATH + "dst_net" + tunnel_id;
			let manual_path = base_path + "_manual";
			ctx.popen("mkdir -p " + DOMAIN_PATH);
			ctx.popen("rm -f " + base_path);
			fs.writefile(manual_path, to.domain_list);
			ctx.popen("ln -sf " + manual_path + " " + base_path);
		} else if (!to.manual && type(to.url) == "string") {
			c.set("route_policy", sid, "to_manual", "0");
			c.set("route_policy", sid, "to_url", to.url);
			c.set("route_policy", sid, "to", prefix + tunnel_id);
			c.set("route_policy", sid, "to_list_external", "/etc/domain_mac_list/dst_net" + tunnel_id);
			apply_url_policy(ctx, tunnel_id, to.url);
		} else {
			c.delete("route_policy", sid);
			return { ok: false, err: e_missing() };
		}
	} else if (to.type != "default") {
		c.delete("route_policy", sid);
		return { ok: false, err: e_missing() };
	}

	if (type(params.options) == "object") {
		params.options.tunnel_id = tunnel_id;
		save_options(ctx, params.options, c, sid);
	}
	return { ok: true, sid: sid };
}

// ---- read side: build the rich tunnel list the dashboard expects -------------
function get_tunnel_list(c, rule_type, tunnel_id, ctx) {
	let rules = [];
	let configed_instances = {};
	c.foreach("route_policy", rule_type, function(s) {
		if (tunnel_id != null && tunnel_id != ((s.tunnel_id != null) ? int(s.tunnel_id) : null)) return;

		let from_type;
		if (s.from_type == "interface") from_type = "interface";
		else if (type(s.from) == "string" && index(s.from, "!src_mac") >= 0) from_type = "exclude_mac";
		else if (type(s.from) == "string" && index(s.from, "src_mac") >= 0) from_type = "mac";
		else from_type = "default";

		let to_type;
		if (type(s.to) == "string" && index(s.to, "!dst_net") >= 0) to_type = "exclude_domain";
		else if (type(s.to) == "string" && index(s.to, "dst_net") >= 0) to_type = "domain";
		else to_type = "default";

		let domain_file = null;
		if (to_type == "domain" || to_type == "exclude_domain") {
			domain_file = DOMAIN_PATH + "dst_net" + s.tunnel_id + "_manual";
			if (!fs.access(domain_file) && s.to_manual != "0") domain_file = DOMAIN_PATH + "dst_net" + s.tunnel_id;
		}

		let domain_list_len = null;
		if (s.to_url && s.to_list_external) {
			if (fs.access(s.to_list_external)) {
				let wc = ctx ? ctx.popen(`wc -l ${s.to_list_external} 2>/dev/null | awk '{print $1}'`) : null;
				domain_list_len = (wc && trim(wc) != "") ? int(trim(wc)) : 0;
			} else {
				domain_list_len = 0;
			}
		}

		if (s.via && s.options_in_used == "1")
			configed_instances[s.via] = (s.tunnel_id != null) ? int(s.tunnel_id) : null;

		let options = null;
		if (rule_type == "rule") {
			options = {
				mtu: (s.mtu != null) ? int(s.mtu) : null,
				masq: s.masq != "0",
				service_policy: s.service_policy == "1",
				local_access: s.local_access == "1",
				options_in_used: s.options_in_used == "1",
				tunnel_using_options: (s.via != null) ? configed_instances[s.via] : null
			};
		}

		let candidate_configs = null;
		if (rule_type == "rule" && (s.via_type == "openvpn" || s.via_type == "wireguard")) {
			let profiles_path = profile_build_path((s.tunnel_id != null) ? int(s.tunnel_id) : null, s.profiles);
			candidate_configs = profile_read_grouped(profiles_path);
		}

		push(rules, {
			id: s[".name"],
			name: s.name,
			enabled: s.enabled == "1",
			tunnel_id: (s.tunnel_id != null) ? int(s.tunnel_id) : null,
			killswitch: s.killswitch == "1",
			from: {
				type: from_type,
				interface_list: (from_type == "interface") ? s.from : null,
				mac_list: s.from_mac
			},
			to: {
				type: to_type,
				manual: s.to_manual != "0",
				url: s.to_url,
				domain_list: (domain_file && fs.access(domain_file)) ? (fs.readfile(domain_file) ?? "") : "",
				domain_list_len: (domain_list_len != null) ? int(domain_list_len) : 0
			},
			via: {
				type: s.via_type,
				via: s.via ?? "",
				configs: candidate_configs
			},
			options: options
		});
	});
	return rules;
}

// ---- status helpers ---------------------------------------------------------
function get_if_statistics(ifname) {
	let p = `/sys/class/net/${ifname}/statistics/`;
	if (fs.access(p)) {
		let rx = fs.readfile(p + "rx_bytes"), tx = fs.readfile(p + "tx_bytes");
		return { rx_bytes: rx ? trim(rx) : null, tx_bytes: tx ? trim(tx) : null };
	}
	return null;
}

function get_connect_status(ctx, ifname) {
	let st = ctx.ubus.call("network.interface." + ifname, "status") ?? {};
	if (st.up) return STATUS_CONNECTED;
	if (st.pending) return STATUS_CONNECTING;
	return STATUS_NOT_START;
}

function get_novpn_status(ctx) {
	// DEGRADE: original asked gl-session system.get_network_status; approximate with
	// mainline netifd (first up WAN-ish iface == connected, else connecting).
	for (let ifn in ["wan", "wwan", "wan6"]) {
		let s = ctx.ubus.call("network.interface." + ifn, "status") ?? {};
		if (s.up) return { interface: ifn, status: STATUS_CONNECTED };
	}
	return { interface: null, status: STATUS_CONNECTING };
}

function set_remote_status(res, remote) {
	let candidates = [];
	if (type(remote) == "string") remote = [ remote ];
	if (type(remote) != "array") return candidates;
	let domains = [], seen = {};
	for (let item in remote) {
		let hp = split_host_port(item);
		let domain = hp[0], port = (hp[1] != null) ? int(hp[1]) : null;
		if (domain && domain != "" && !seen[domain]) { seen[domain] = true; push(domains, domain); }
		if (domain && domain != "" && port != null) push(candidates, { domain: domain, port: port });
		res.port = port;
	}
	res.domain = domains;
	return candidates;
}

function set_active_remote_status(ctx, res, candidates, vpn_client) {
	if (type(candidates) != "array" || length(candidates) == 0) return;
	if (!safe_ifname(vpn_client)) return;
	let pids = {};
	let ps = ctx.popen("ps w 2>/dev/null | grep '[o]penvpn'") ?? "";
	for (let line in split(ps, "\n")) {
		let pm = match(line, /^[ \t]*([0-9]+)/);
		let dm = match(line, /--dev[ \t]+([^ \t]+)/);
		if (pm && dm && dm[1] == vpn_client) pids[pm[1]] = true;
	}
	let has = false; for (let k in pids) { has = true; break; }
	if (!has) return;
	let active_port = null;
	let ns = ctx.popen("netstat -tnpau 2>/dev/null | grep openvpn") ?? "";
	for (let line in split(ns, "\n")) {
		let pm = match(line, /([0-9]+)\/openvpn/);
		if (!pm || !pids[pm[1]]) continue;
		let fm = match(line, /^[^ ]+[ \t]+[^ ]+[ \t]+[^ ]+[ \t]+[^ ]+[ \t]+([^ ]+)/);
		let host = null, port = null;
		if (fm) { let hp = match(fm[1], /^(.+):([0-9]+)$/); if (hp) { host = hp[1]; port = int(hp[2]); } }
		if (port == null) continue;
		for (let cand in candidates) {
			if (cand.port == port) {
				active_port = cand.port;
				if (cand.domain == host) { res.domain = [ cand.domain ]; res.port = cand.port; return; }
			}
		}
	}
	if (active_port != null) res.port = active_port;
}

function get_if_status(c, sid, res, ctx) {
	let via_type = c.get("route_policy", sid, "via_type");
	if (via_type == "novpn") {
		if (res.enabled) { let ns = get_novpn_status(ctx); res.via = ns.interface; res.status = ns.status; }
		return res;
	}
	let group_id = c.get("route_policy", sid, "group_id");
	let peer_id = c.get("route_policy", sid, "peer_id");
	let client_id = c.get("route_policy", sid, "client_id");
	res.group_id = (group_id != null) ? int(group_id) : null;
	res.peer_id = (peer_id != null) ? int(peer_id) : null;
	res.client_id = (client_id != null) ? int(client_id) : null;

	let vpn_client = c.get("route_policy", sid, "via");
	if (type(vpn_client) != "string") {
		if (res.enabled) res.status = STATUS_CONNECTING;
		return res;
	}
	res.via = vpn_client;

	let remote_candidates = null;
	let c2 = ctx.uci.cursor();   // fresh cursor for the nested lookup (avoid reentrancy)
	if (index(vpn_client, "wgclient") >= 0) {
		res.type = "wireguard";
		c2.foreach("wireguard", "peers", function(s) {
			let mm = match(s[".name"], /[0-9]+/);
			let peer = (s.peer_id != null) ? `${s.peer_id}` : (mm ? mm[0] : null);
			if (s.group_id == group_id && peer == peer_id) {
				res.peer_name = s.name;
				if (s.end_point) {
					let hp = split_host_port(s.end_point);
					res.domain = [ hp[0] ];
					res.port = (hp[1] != null) ? int(hp[1]) : null;
				}
			}
		});
	} else {
		res.type = "openvpn";
		c2.foreach("ovpnclient", "clients", function(s) {
			if (s.group_id == group_id && s.client_id == client_id) {
				res.peer_name = s.name;
				remote_candidates = set_remote_status(res, s.remote);
			}
		});
	}

	if (!res.enabled) return res;
	res.status = get_connect_status(ctx, vpn_client);
	if (res.status == STATUS_NOT_START && res.enabled) res.status = STATUS_CONNECTING;
	set_active_remote_status(ctx, res, remote_candidates, vpn_client);

	let st = get_if_statistics(vpn_client);
	res.rx_bytes = (st && st.rx_bytes != null) ? int(st.rx_bytes) : null;
	res.tx_bytes = (st && st.tx_bytes != null) ? int(st.tx_bytes) : null;

	if (safe_ifname(vpn_client)) {
		let v4 = ctx.popen(`ip -4 -o addr show dev ${vpn_client} 2>/dev/null | awk '{print $4}' | cut -d/ -f1 | head -n1`);
		res.ipv4 = v4 ?? "";
		let v6raw = ctx.popen(`ip -6 -o addr show dev ${vpn_client} scope global 2>/dev/null | awk '{print $4}' | cut -d/ -f1`);
		if (v6raw) {
			let arr = [];
			for (let l in split(v6raw, "\n")) { let t = trim(l); if (t != "") push(arr, t); }
			if (length(arr) > 0) res.ipv6 = arr;
		}
	}
	return res;
}

function build_status(c, ctx) {
	let res = [];
	let mode = c.get("route_policy", "global", "mode");
	c.foreach("route_policy", "rule", function(s) {
		let tunnel = {
			name: s.name,
			tunnel_id: (s.tunnel_id != null) ? int(s.tunnel_id) : null,
			type: s.via_type,
			enabled: s.enabled == "1"
		};
		get_if_status(c, s[".name"], tunnel, ctx);
		push(res, tunnel);
	});
	return { mode: (mode != null) ? int(mode) : 0, status_list: res };
}

function notify_vpn_status(ctx) {
	// DEGRADE: best-effort push onto gl-session's event bus; ignored when absent.
	let c = ctx.uci.cursor();
	let status = build_status(c, ctx);
	try { ctx.ubus.call("gl-session", "notify", { name: "vpnclient.status", data: status }); } catch (e) {}
}

// ---- native policy-routing applier (replaces vpn-client init + tunnel-switch)
function flush_policy(ctx) {
	ctx.popen("nft delete table inet gl_vpn_client 2>/dev/null");
	for (let i = 1; i <= MAX_T; i++) {
		ctx.popen(`while ip rule del pref ${PREF_BASE + i} 2>/dev/null; do : ; done`);
		ctx.popen(`ip route flush table ${TABLE_BASE + i} 2>/dev/null`);
	}
}

function apply_policy(ctx) {
	// DEGRADE: GL's /etc/init.d/vpn-client + tunnel-switch.sh are absent. We natively
	// program src-MAC -> fwmark -> per-tunnel routing table + masquerade for enabled
	// wireguard/openvpn rules whose selector is from.type=="mac" (the common case),
	// with killswitch blackholing when the tunnel is down. Everything else stays
	// store-only (config round-trips; enforcement skipped) since it needs the GL
	// domain resolver / firewall-zone map that mainline lacks here.
	if (!tool_available(ctx, "ip") || !tool_available(ctx, "nft")) return;   // DEGRADE: store-only
	flush_policy(ctx);

	let c = ctx.uci.cursor();
	let sets = [], mark_rules = [], nat_rules = [];
	let counter = { i: 0 };
	c.foreach("route_policy", "rule", function(s) {
		if (s.enabled != "1") return;
		if (counter.i >= MAX_T) return;
		let via_type = s.via_type;
		if (via_type != "wireguard" && via_type != "openvpn") return;   // novpn/autovpn: store-only

		let from_is_mac = (type(s.from) == "string" && index(s.from, "src_mac") >= 0 && index(s.from, "!src_mac") < 0);
		let macs = (type(s.from_mac) == "array") ? s.from_mac : ((type(s.from_mac) == "string") ? [ s.from_mac ] : []);
		if (!from_is_mac || length(macs) == 0) return;   // only MAC-source rules enforced natively
		let clean = [];
		for (let m in macs) if (type(m) == "string" && match(m, MAC_RE)) push(clean, lc(m));
		if (length(clean) == 0) return;

		counter.i++;
		let idx = counter.i;
		let tid = (s.tunnel_id != null) ? int(s.tunnel_id) : idx;
		let mark = idx, table = TABLE_BASE + idx, pref = PREF_BASE + idx;
		let setname = "mac_t" + tid;
		let ifn = safe_ifname(s.via);
		let up = ifn ? (get_connect_status(ctx, ifn) == STATUS_CONNECTED) : false;
		let killswitch = s.killswitch == "1";

		if (ifn && up) {
			ctx.popen(`ip route replace default dev ${ifn} table ${table} 2>/dev/null`);
		} else if (killswitch) {
			ctx.popen(`ip route replace blackhole default table ${table} 2>/dev/null`);
		}
		// tunnel down + no killswitch -> empty table, traffic falls through to main.
		ctx.popen(`ip rule add fwmark ${mark} lookup ${table} pref ${pref} 2>/dev/null`);

		push(sets, `\tset ${setname} {\n\t\ttype ether_addr\n\t\telements = { ${join(", ", clean)} }\n\t}`);
		push(mark_rules, `\t\tether saddr @${setname} meta mark set ${mark}`);
		if (ifn && up && s.masq != "0") push(nat_rules, `\t\toifname "${ifn}" masquerade`);
	});

	if (length(sets) == 0) return;   // nothing enforceable natively

	let ruleset =
		"table inet gl_vpn_client {\n" +
		join("\n", sets) + "\n" +
		"\tchain mark {\n" +
		"\t\ttype filter hook prerouting priority mangle; policy accept;\n" +
		join("\n", mark_rules) + "\n" +
		"\t}\n" +
		((length(nat_rules) > 0) ?
			"\tchain nat {\n" +
			"\t\ttype nat hook postrouting priority srcnat; policy accept;\n" +
			join("\n", nat_rules) + "\n" +
			"\t}\n" : "") +
		"}\n";
	fs.writefile("/tmp/gl-vpn-client.nft", ruleset);
	ctx.popen("nft -f /tmp/gl-vpn-client.nft 2>/dev/null");
}

// ---- generate a default tunnel (used by start_random_client w/o tunnel_id) ---
function generate_tunnel_instance(ctx) {
	let c = ctx.uci.cursor();
	let params = { from: { type: "default" }, to: { type: "default" }, via: { type: "autovpn" }, name: "Tunnel 1", options: { killswitch: true } };
	let tunnel_id = generate_tunnel_id(c);
	params.tunnel_id = tunnel_id;
	params._write_profiles = true;
	let r = add_tunnel_config(c, params, ctx);
	if (!r.ok) return null;
	c.commit("route_policy");
	system("sync 2>/dev/null");
	let list = get_tunnel_list(c, "rule", tunnel_id, ctx);
	return list ? list[0] : null;
}

function wait_for_if_up(ctx, sid, c) {
	// DEGRADE: the GL tunnel-switch daemon that actually brought up the wireguard/
	// openvpn iface is absent, and add_tunnel_config never learns the iface name
	// (route_policy `via` is populated by that daemon). Best-effort: if an iface is
	// already known, ifup + short poll; otherwise treat as applied (store-only) so we
	// don't emit a spurious connect-fail for config the app successfully saved.
	let ifn = c.get("route_policy", sid, "via");
	if (type(ifn) != "string" || !safe_ifname(ifn)) return true;
	ctx.popen(`ifup ${ifn} 2>/dev/null; ip link set ${ifn} up 2>/dev/null`);
	let i = 6;
	while (i > 0) {
		i--;
		let st = ctx.ubus.call("network.interface." + ifn, "status") ?? {};
		if (st.up) return true;
		ctx.popen("sleep 1");
	}
	return false;
}

// ---- per-client vpn-using matching (get_vpn_using_status) --------------------
function collect_client_macs(ctx) {
	let set = {};
	let raw = fs.readfile("/tmp/dhcp.leases");
	if (raw) for (let line in split(trim(raw), "\n")) {
		let f = split(line, " ");
		if (length(f) >= 2 && match(f[1], /^[0-9a-fA-F:]{17}$/)) set[lc(f[1])] = true;
	}
	let neigh = ctx.popen("ip neigh show 2>/dev/null") ?? "";
	for (let line in split(neigh, "\n")) {
		let m = match(line, /([0-9a-fA-F:]{17})/);
		if (m) set[lc(m[1])] = true;
	}
	let out = [];
	for (let k in set) push(out, k);
	return out;
}

function match_from(from, mac, typ) {
	if (from.type == "exclude_mac" && type(from.mac_list) == "array") {
		for (let item in from.mac_list) if (item == mac) return false;
	}
	if (from.type == "mac" && type(from.mac_list) == "array") {
		let found = false;
		for (let item in from.mac_list) if (item == mac) found = true;
		return found;
	}
	if (from.type == "interface") {
		let list = (type(from.interface_list) == "array") ? from.interface_list :
		           ((type(from.interface_list) == "string") ? [ from.interface_list ] : []);
		for (let item in list) {
			if (item == "lan" && (typ == 0 || typ == 1 || typ == 2 || typ == 9 || typ == 11)) return true;
			else if (item == "guest" && (typ == 3 || typ == 4 || typ == 10 || typ == 12)) return true;
			else if (item == "iot" && (typ == 15 || typ == 16)) return true;
			else if (item == "wan" && typ == 7) return true;
		}
		return false;
	}
	return true;
}

function match_via(ctx, rule) {
	let via = rule ? rule.via : null;
	if (!via) return { match: false, status: STATUS_NOT_EFFORT };
	if (via.type == "novpn") return { match: true, status: STATUS_NOT_USE };
	if (via.via && get_connect_status(ctx, via.via) == STATUS_CONNECTED) return { match: true, status: STATUS_USE_VPN };
	if (rule.killswitch && rule.options && rule.options.options_in_used) return { match: true, status: STATUS_VPN_DOWN };
	return { match: false, status: STATUS_NOT_EFFORT };
}

function match_mac_and_rules(c, ctx, rules, mac, typ) {
	let res = STATUS_NOT_USE;
	let default_enabled = c.get("route_policy", "@default[0]", "enabled");
	for (let rule in rules) {
		if (rule.enabled && match_from(rule.from ?? {}, mac, typ)) {
			let mv = match_via(ctx, rule);
			if (mv.match) return mv.status;
			else res = mv.status;
		}
	}
	if (res == STATUS_NOT_EFFORT && default_enabled == "0") res = STATUS_VPN_DOWN_AND_WAN_BLOCK;
	return res;
}

// ---- config-list fallback (gl-session fan-out may be absent) ------------------
function build_wg_list(c) {
	let groups = {};
	c.foreach("wireguard", "peers", function(s) {
		if (s.group_id == null) return;
		let mm = match(s[".name"], /[0-9]+/);
		let pid = (s.peer_id != null) ? int(s.peer_id) : (mm ? int(mm[0]) : null);
		let key = "" + s.group_id;
		groups[key] ??= { group_id: int(s.group_id), peers: [] };
		push(groups[key].peers, { peer_id: pid, name: s.name, end_point: s.end_point });
	});
	let out = [];
	for (let k in groups) push(out, groups[k]);
	return out;
}

function build_ov_list(c) {
	let groups = {};
	c.foreach("ovpnclient", "clients", function(s) {
		if (s.group_id == null) return;
		let key = "" + s.group_id;
		groups[key] ??= { group_id: int(s.group_id), clients: [] };
		push(groups[key].clients, { client_id: (s.client_id != null) ? int(s.client_id) : null, name: s.name });
	});
	let out = [];
	for (let k in groups) push(out, groups[k]);
	return out;
}

// =============================================================================
export default {
	add_tunnel: function(params, ctx) {
		let c = ctx.uci.cursor();
		if (type(params.from) != "object" || type(params.to) != "object") return e_missing();
		if (!params.options) params.options = { killswitch: true };
		let tunnel_id = generate_tunnel_id(c);
		params.tunnel_id = tunnel_id;
		params._write_profiles = true;
		let r = add_tunnel_config(c, params, ctx);
		if (!r.ok) return r.err;
		c.commit("route_policy");
		system("sync 2>/dev/null");
		if (params.enabled) apply_policy(ctx);
		notify_vpn_status(ctx);
		return { tunnel_id: tunnel_id };
	},

	set_tunnel: function(params, ctx) {
		let c = ctx.uci.cursor();
		// DROPPED: tap-s2s conflict guard (that layer-2 stack is absent here).
		let f = get_tunnel_sid(c, params.tunnel_id);
		let sid = f.sid, index2 = f.index;
		if (!sid) return e_invalid();
		let list = get_tunnel_list(c, "rule", params.tunnel_id, ctx);
		if (!list || type(list[0]) != "object") return e_invalid();
		let config = list[0];
		config.profiles = c.get("route_policy", sid, "profiles");

		let via_type = (type(params.via) == "object") ? params.via.type : null;
		if (params.via != null && type(params.via) != "object") return e_invalid();
		if (type(params.via) == "object" && (via_type == "wireguard" || via_type == "openvpn")) {
			if (type(params.via.configs) != "array") return e_missing();
			config._write_profiles = true;
		}

		let old_enabled = config.enabled;
		if (type(params.to) == "object" && type(config.to) == "object" && params.to.url == null)
			params.to.url = config.to.url;
		for (let k in params) if (k != "_write_profiles") config[k] = params[k];
		if (via_type == "novpn") config.profiles = null;

		let r = add_tunnel_config(c, config, ctx);
		if (!r.ok) return r.err;
		let new_sid = r.sid;
		if (index2 != null) reorder_section(c, new_sid, index2);
		c.delete("route_policy", sid);
		c.commit("route_policy");
		system("sync 2>/dev/null");
		// DEGRADE: gl_screen event hook dropped (no /proc/gl-hw-info/screen).
		notify_vpn_status(ctx);
		if (!old_enabled && !params.enabled) return { tunnel_id: params.tunnel_id };
		apply_policy(ctx);
		return { tunnel_id: params.tunnel_id };
	},

	get_tunnel: function(params, ctx) {
		let c = ctx.uci.cursor();
		return {
			// (original compared a uci string to the number 1 -> always false; use "1")
			global_enabled: (c.get("route_policy", "global", "enabled") == "1"),
			default_tunnels: get_tunnel_list(c, "default", null, ctx),
			tunnels: get_tunnel_list(c, "rule", null, ctx)
		};
	},

	remove_tunnel: function(params, ctx) {
		let res = {};
		let c = ctx.uci.cursor();
		let f = get_tunnel_sid(c, params.tunnel_id);
		let sid = f.sid;
		if (sid) {
			let rule_type = c.get("route_policy", sid);
			if (rule_type == "default") return e_invalid();
			c.delete("route_policy", sid);
			fs.unlink("/etc/domain_mac_list/dst_net" + params.tunnel_id);
			profile_remove(params.tunnel_id);
			res.status = "success";
		}
		c.commit("route_policy");
		system("sync 2>/dev/null");
		if (res.status == "success" && length(get_tunnel_list(c, "rule", null, ctx)) == 0)
			this.set_default_tunnel({ enabled: true }, ctx);
		// DEGRADE: gl_screen event hook dropped.
		apply_policy(ctx);
		return res;
	},

	order_tunnel: function(params, ctx) {
		let res = {};
		let c = ctx.uci.cursor();
		let index2 = null;
		c.foreach("route_policy", "default", function(s) { index2 = s[".index"]; });
		if (index2 != null && type(params.id_list) == "array") {
			for (let id in params.id_list) {
				if (type(id) == "string" && c.get("route_policy", id) == "rule") {
					reorder_section(c, id, index2 + 1);
					index2 = index2 + 1;
				}
			}
		} else {
			return e_invalid();
		}
		c.commit("route_policy");
		system("sync 2>/dev/null");
		apply_policy(ctx);
		return res;
	},

	set_single_mac: function(params, ctx) {
		let res = {};
		let c = ctx.uci.cursor();
		let flags = { need_restart: false };
		if (type(params.mac) != "string" || type(params.tunnel_id_list) != "array") return e_invalid();
		c.foreach("route_policy", "rule", function(s) {
			if (!(type(s.from) == "string" && index(s.from, "src_mac") >= 0) || type(s.from_mac) != "array") return;
			let found = false;
			for (let v in params.tunnel_id_list) if (s.tunnel_id != null && int(v) == int(s.tunnel_id)) found = true;
			let list = [];
			for (let m in s.from_mac) push(list, m);
			let idx = null;
			for (let i = length(list) - 1; i >= 0; i--) if (list[i] == params.mac) idx = i;
			if (found) {
				if (idx == null) push(list, params.mac);
			} else if (idx != null && length(list) > 1) {
				splice(list, idx, 1);
			}
			c.set("route_policy", s[".name"], "from_mac", list);
			if (s.enabled == "1") flags.need_restart = true;
		});
		c.commit("route_policy");
		system("sync 2>/dev/null");
		if (flags.need_restart) apply_policy(ctx);
		return res;
	},

	get_connection_methods: function(params, ctx) {
		let c = ctx.uci.cursor();
		let zones = [];
		c.foreach("firewall", "zone", function(s) {
			if (s.family == "ipv6" || s.name == "zerotier" || s.name == "tailscale0" ||
			    s.name == "wpan" || s.name == "mptun" || s.name == "wg_s2s") return;
			if (type(s.name) == "string" && (index(s.name, "wgclient") >= 0 || index(s.name, "ovpnclient") >= 0 || index(s.name, "wan_all") >= 0)) return;
			if (type(s.name) == "string") push(zones, s.name);
		});
		return { connection_methods: zones };
	},

	get_all_config_list: function(params, ctx) {
		// Try GL's cross-module fan-out first (present iff those modules run on-device).
		let ov = ctx.ubus.call("gl-session", "call", { module: "ovpn-client", func: "get_all_config_list", params: {} });
		let wg = ctx.ubus.call("gl-session", "call", { module: "wg-client", func: "get_all_config_list", params: {} });
		let wireguard = (wg && wg.result) ? wg.result.config_list : null;
		let openvpn = (ov && ov.result) ? ov.result.config_list : null;
		// DEGRADE: fall back to reading the uci stores directly when unavailable.
		let c = ctx.uci.cursor();
		if (wireguard == null) wireguard = build_wg_list(c);
		if (openvpn == null) openvpn = build_ov_list(c);
		return { configs: { wireguard: wireguard, openvpn: openvpn } };
	},

	get_status: function(params, ctx) {
		// PRIORITY path (dashboard poll). Never crashes; empty status_list when no rules.
		let c = ctx.uci.cursor();
		return build_status(c, ctx);
	},

	set_options: function(params, ctx) {
		let res = save_options(ctx, params, null, null);
		apply_policy(ctx);
		return res;
	},

	check_domain_online: function(params, ctx) {
		if (type(params.url) != "string") return e_missing();
		return download_domain_list(ctx, params.url);
	},

	set_default_tunnel: function(params, ctx) {
		let c = ctx.uci.cursor();
		ensure_section(c, "@default[0]", "default");
		c.set("route_policy", "@default[0]", "enabled", params.enabled ? "1" : "0");
		c.commit("route_policy");
		system("sync 2>/dev/null");
		apply_policy(ctx);
		return {};
	},

	start_random_client: function(params, ctx) {
		let c = ctx.uci.cursor();
		params = params ?? {};
		let group_id = (params.group_id != null) ? int(params.group_id) : null;
		if (group_id == null) return e_missing();
		let target_peer_id = (params.peer_id != null) ? int(params.peer_id) : null;

		let config;
		if (params.tunnel_id == null) {
			config = generate_tunnel_instance(ctx);
		} else {
			let tid = int(params.tunnel_id);
			if (!tid) return e_invalid();
			let list = get_tunnel_list(c, "rule", tid, ctx);
			config = list ? list[0] : null;
		}
		if (type(config) != "object" || config.tunnel_id == null) return e_invalid();

		let id_list = [];
		c.foreach("wireguard", "peers", function(s) {
			let mm = match(s[".name"], /[0-9]+/);
			let pid = mm ? int(mm[0]) : null;
			if (s.group_id != null && int(s.group_id) == group_id && pid != null && (!target_peer_id || target_peer_id == pid))
				push(id_list, pid);
		});
		if (length(id_list) == 0) return { tunnel_id: config.tunnel_id };

		config.enabled = true;
		config._write_profiles = true;
		let of = get_tunnel_sid(c, config.tunnel_id);
		let old_sid = of.sid, old_index = of.index;
		config.via = { type: "wireguard", configs: [ { group_id: group_id, id_list: id_list } ] };

		let r = add_tunnel_config(c, config, ctx);
		if (!r.ok) {
			return {
				tunnel_id: config.tunnel_id,
				err_code: (r.err && r.err.err_code) ?? -1,
				err_msg: (r.err && r.err.err_msg) ?? "vpn client connect failed"
			};
		}
		let new_sid = r.sid;
		if (old_sid && old_sid != new_sid && params.tunnel_id != null && old_index != null)
			reorder_section(c, new_sid, old_index);
		if (old_sid && old_sid != new_sid) c.delete("route_policy", old_sid);
		c.commit("route_policy");
		system("sync 2>/dev/null");
		apply_policy(ctx);
		let up = wait_for_if_up(ctx, new_sid, c);
		notify_vpn_status(ctx);
		if (!up)
			return { tunnel_id: config.tunnel_id, err_code: -1, err_msg: "vpn client connect failed" };
		return { tunnel_id: config.tunnel_id };
	},

	stop: function(params, ctx) {
		let c = ctx.uci.cursor();
		let st = { route_changed: false, profiles_changed: false, changed_ids: [] };
		if (!params.group_id || !params.type) return e_missing();
		let stop_gid = int(params.group_id);
		if (!stop_gid) return e_invalid();
		let stop_id = int((params.type == "wireguard") ? params.peer_id : params.client_id);
		c.foreach("route_policy", "rule", function(s) {
			if (s.via_type != params.type) return;
			let profile_path = profile_build_path((s.tunnel_id != null) ? int(s.tunnel_id) : null, s.profiles);
			if (!profile_path) return;
			let result = profile_filter(ctx, profile_path, function(gid, id) { return gid != stop_gid || id != stop_id; });
			if (!result.changed) return;
			st.profiles_changed = true;
			let id_field = (params.type == "wireguard") ? "peer_id" : "client_id";
			if (length(result.valid) == 0) {
				c.delete("route_policy", s[".name"], "group_id");
				c.delete("route_policy", s[".name"], id_field);
				c.delete("route_policy", s[".name"], "via_type");
				c.set("route_policy", s[".name"], "enabled", "0");
				st.route_changed = true;
				let tid = (s.tunnel_id != null) ? int(s.tunnel_id) : null;
				if (tid != null) push(st.changed_ids, tid);
			} else {
				let current_gid = (s.group_id != null) ? int(s.group_id) : null;
				let current_id = (s[id_field] != null) ? int(s[id_field]) : null;
				if (current_gid == stop_gid && current_id == stop_id) {
					let nc = result.valid[0];
					c.set("route_policy", s[".name"], "group_id", "" + nc.group_id);
					c.set("route_policy", s[".name"], id_field, "" + nc.id);
					st.route_changed = true;
					if (s.enabled == "1") { let tid = (s.tunnel_id != null) ? int(s.tunnel_id) : null; if (tid != null) push(st.changed_ids, tid); }
				}
			}
		});
		if (st.route_changed) c.commit("route_policy");
		if (st.route_changed || st.profiles_changed) system("sync 2>/dev/null");
		if (st.route_changed) apply_policy(ctx);   // DEGRADE: replaces kill_tunnel_switch + init restart
		return {};
	},

	turn_off_all_vpn_tunnel: function(params, ctx) {
		let c = ctx.uci.cursor();
		let ifnames = [];
		let flags = { need_reload: false };
		c.foreach("route_policy", "rule", function(s) {
			if (s.enabled == "1") {
				c.set("route_policy", s[".name"], "enabled", "0");
				if (type(s.via) == "string") push(ifnames, s.via);
				flags.need_reload = true;
			}
		});
		let de = c.get("route_policy", "@default[0]", "enabled");
		if (de != "1") {
			ensure_section(c, "@default[0]", "default");
			c.set("route_policy", "@default[0]", "enabled", "1");
			flags.need_reload = true;
		}
		if (flags.need_reload) {
			for (let iface in ifnames) if (safe_ifname(iface)) ctx.popen(`ip link set ${iface} down 2>/dev/null`);
			c.commit("route_policy");
			apply_policy(ctx);   // DEGRADE: replaces /usr/bin/rtp2.sh
		}
		return {};
	},

	set_tap_s2s: function(params, ctx) {
		// DEGRADE + DROP guards: the TAP-S2S layer-2 bridge relies on GL's
		// set_tap_s2s_client.sh / mptun / reset_network_utils (all absent). The mptun
		// AW_status conflict guard is dropped. We only round-trip the route_policy
		// is_tap_s2s flag; the actual bridge is NOT established.
		let c = ctx.uci.cursor();
		if (params.group_id == null || params.client_id == null) return e_missing();
		let config = params.group_id + "_" + params.client_id;
		let mode = c.get("ovpnclient", config, "mode");
		if (mode != "tap-s2s" || type(params.enabled) != "bool") return e_invalid();
		ensure_section(c, "global", "global");
		c.set("route_policy", "global", "is_tap_s2s", params.enabled ? "1" : "0");
		c.commit("route_policy");
		system("sync 2>/dev/null");
		return {};
	},

	get_vpn_using_status: function(params, ctx) {
		let c = ctx.uci.cursor();
		let client_list = [];
		let rules = get_tunnel_list(c, "rule", null, ctx);
		// DEGRADE: GL's client tracker (get_client_data_by_socket) is absent; derive
		// the MAC set from DHCP leases + neighbour table. Per-client zone "type" is
		// unavailable, so assume LAN (0) -- exact for MAC-scoped rules, and interface-
		// scoped rules resolve as if the client were on lan.
		let macs = collect_client_macs(ctx);
		for (let mac in macs) push(client_list, { mac: mac, status: match_mac_and_rules(c, ctx, rules, mac, 0) });
		return client_list;
	},

	check_vpn_config: function(params, ctx) {
		let c = ctx.uci.cursor();
		let st = { need_apply: false, route_changed: false, profiles_changed: false };
		let vpn_index = build_vpn_client_index(c);
		c.foreach("route_policy", "rule", function(s) {
			if (s.via_type != "wireguard" && s.via_type != "openvpn") return;
			let id_field = (s.via_type == "wireguard") ? "peer_id" : "client_id";
			let current_group_id = (s.group_id != null) ? int(s.group_id) : null;
			let current_id = (s[id_field] != null) ? int(s[id_field]) : null;
			let tunnel_id = (s.tunnel_id != null) ? int(s.tunnel_id) : null;
			let profile_path = profile_build_path(tunnel_id, s.profiles);
			let valid = [], valid_count = 0;
			if (profile_path) {
				let result = profile_filter(ctx, profile_path, function(gid, id) { return vpn_client_exists(vpn_index, s.via_type, gid, id); });
				valid = result.valid;
				valid_count = length(valid);
				if (result.changed) st.profiles_changed = true;
			}
			let current_still_valid = false;
			if (current_group_id != null && current_id != null)
				current_still_valid = vpn_client_exists(vpn_index, s.via_type, current_group_id, current_id);
			if (current_still_valid) return;
			if (valid_count == 0) {
				if (s.enabled == "1") st.need_apply = true;
				c.delete("route_policy", s[".name"], "group_id");
				c.delete("route_policy", s[".name"], id_field);
				c.delete("route_policy", s[".name"], "via_type");
				c.set("route_policy", s[".name"], "enabled", "0");
				st.route_changed = true;
			} else {
				let sole = valid[0];
				c.set("route_policy", s[".name"], "group_id", "" + sole.group_id);
				c.set("route_policy", s[".name"], id_field, "" + sole.id);
				let opposite = (s.via_type == "wireguard") ? "client_id" : "peer_id";
				c.delete("route_policy", s[".name"], opposite);
				st.route_changed = true;
				if (s.enabled == "1") st.need_apply = true;
			}
		});
		if (st.route_changed) c.commit("route_policy");
		if (st.route_changed || st.profiles_changed) system("sync 2>/dev/null");
		if (st.need_apply) apply_policy(ctx);   // DEGRADE: original deferred via ngx.timer
		return {};
	}
};
