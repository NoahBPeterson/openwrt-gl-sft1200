// gl-ucode object: wg_client  (port of decompiled-spec/rpc/wg_client.lua)
// WireGuard client helpers. The real config CRUD + the third-party VPN provider
// cloud clients (Surfshark / IPVanish / ExpressVPN ...) live in a closed .so
// (gl.wg_client / gl.expressvpn), absent on mainline. Split by value:
//   * gen_key       - REIMPLEMENTED via `wg genkey` + `wg pubkey` (ctx.popen).
//   * get_max_client- REIMPLEMENTED via `ubus system info` + `df -k /`.
//   * provider_*    - STUBBED: return {err_code:-1, err_msg:"unsupported provider"}.
//   * expressvpn_*  - STUBBED: GL-Cloud co-branded wizard, disabled/unsupported.
// DEGRADE: gl.vpn_err_code table absent -> generic err_code:-1 for stubs.
//          /proc/gl-hw-info/flash_size absent -> flash_total=0 (0.05 reserve no-op).
//          oui.fs.statvfs absent -> flash-available derived from `df -k /`.
// All original method names are preserved (the app may probe them). If the `wg`
// binary is absent, gen_key degrades to an error object, it does not crash.
'use strict';

const fs = require('fs');

// A canonical WireGuard (Curve25519) base64 key: 43 base64 chars + '='.
// dash-free char class on purpose (ucode regex rejects escaped dash in a class).
function is_wg_key(s) {
	return (type(s) == "string") && (match(s, /^[A-Za-z0-9+\/]{43}=$/) != null);
}

function unsupported_provider() {
	// DEGRADE: provider cloud client is closed-source / absent on mainline.
	return { err_code: -1, err_msg: "unsupported provider" };
}

function unsupported_xv() {
	// DEGRADE: ExpressVPN GL-Cloud wizard (gl.expressvpn) is closed / absent.
	return { err_code: -1, err_msg: "expressvpn wizard unsupported" };
}

// ---- WireGuard client CRUD + tunnel (reimplemented over OpenWrt-native wireguard) ----
// The closed gl.wg_client .so kept configs in an internal store and drove a `wgclient`
// interface. We store saved tunnels in our own uci config `gl_wgclient` (group +
// peers sections) and realize the ACTIVE one as a mainline `network.wgclient`
// (proto wireguard) interface + peer. wgclient is already in the wan firewall zone
// (masq + MSS clamp), so LAN clients egress through the tunnel once it's up.
const WGIF   = "wgclient";           // netifd interface + wg device name
// Saved WG configs live in the STANDARD `wireguard` uci store (config group + peers),
// which is exactly what vpn-client.get_all_config_list / get_status read -- so the app's
// unified VPN page sees them. We add an extra `state` section (active group/peer); it's a
// distinct section type, ignored by vpn-client's `peers` foreach.
const STORE  = "wireguard";

function ensure_store(c) {
	if (!fs.access(`/etc/config/${STORE}`)) {
		let t = fs.open(`/etc/config/${STORE}`, "w");
		if (t) t.close();
	}
}

// "host:port" -> {host, port}  (IPv6 endpoints use [addr]:port)
function split_endpoint(ep) {
	let m = match(ep ?? "", /^\[(.+)\]:([0-9]+)$/) || match(ep ?? "", /^(.+):([0-9]+)$/);
	return m ? { host: m[1], port: m[2] } : { host: ep ?? "", port: "51820" };
}

// read a saved peer config (group_id/peer_id) from the store into a flat object
function load_peer(c, group_id, peer_id) {
	let found = null;
	c.foreach(STORE, "peers", function(s) {
		if (s.group_id == `${group_id}` && s.peer_id == `${peer_id}`) found = s;
	});
	return found;
}

// bring the active tunnel up from a saved peer config
function apply_tunnel(ctx, p) {
	let c = ctx.uci.cursor();
	let ep = split_endpoint(p.end_point);
	// interface
	c.set("network", WGIF, "interface");
	c.set("network", WGIF, "proto", "wireguard");
	c.set("network", WGIF, "private_key", p.private_key);
	let addrs = [];
	if (p.address_v4 && p.address_v4 != "") push(addrs, p.address_v4);
	if (p.address_v6 && p.address_v6 != "") push(addrs, p.address_v6);
	c.set("network", WGIF, "addresses", addrs);
	if (p.mtu && p.mtu != "") c.set("network", WGIF, "mtu", p.mtu);
	if (p.dns && p.dns != "") c.set("network", WGIF, "dns", split(p.dns, ","));
	// peer (a wireguard_wgclient section)
	c.delete("network", `${WGIF}_peer`);
	c.set("network", `${WGIF}_peer`, `wireguard_${WGIF}`);
	c.set("network", `${WGIF}_peer`, "public_key", p.public_key);
	if (p.preshared_key && p.preshared_key != "")
		c.set("network", `${WGIF}_peer`, "preshared_key", p.preshared_key);
	c.set("network", `${WGIF}_peer`, "endpoint_host", ep.host);
	c.set("network", `${WGIF}_peer`, "endpoint_port", ep.port);
	c.set("network", `${WGIF}_peer`, "allowed_ips",
		split(p.allowed_ips && p.allowed_ips != "" ? p.allowed_ips : "0.0.0.0/0,::/0", ","));
	c.set("network", `${WGIF}_peer`, "route_allowed_ips", "1");
	c.set("network", `${WGIF}_peer`, "persistent_keepalive",
		(p.persistent_keepalive && p.persistent_keepalive != "") ? p.persistent_keepalive : "25");
	c.commit("network");
	ctx.ubus.call("network", "reload");
	ctx.popen(`/sbin/ifup ${WGIF} >/dev/null 2>&1`);
}

// tear the active tunnel down (keep saved configs)
function down_tunnel(ctx) {
	let c = ctx.uci.cursor();
	ctx.popen(`/sbin/ifdown ${WGIF} >/dev/null 2>&1`);
	c.delete("network", WGIF);
	c.delete("network", `${WGIF}_peer`);
	c.commit("network");
	ctx.ubus.call("network", "reload");
	// The full-tunnel default route replaced the physical uplink's default; netifd's
	// reload won't re-add the uplink route on its own, so re-up every wan-zone uplink
	// to restore the physical default route. Without this the router (and every LAN
	// client) loses internet on VPN disconnect. Re-up only interfaces that actually
	// exist so this is a no-op for absent ones.
	let ups = {};
	c.foreach("firewall", "zone", function(z) {
		if (type(z.name) == "string" && (index(z.name, "wan") >= 0)) {
			let nets = (type(z.network) == "array") ? z.network : ((type(z.network) == "string") ? [z.network] : []);
			for (let n in nets) if (n != WGIF) ups[n] = true;
		}
	});
	// always include the usual uplink names as a safety net
	for (let n in ["wan", "wan6", "internet", "wwan", "tethering"]) ups[n] = true;
	for (let n in ups)
		if (ctx.ubus.call(`network.interface.${n}`, "status") != null)
			ctx.popen(`/sbin/ifup ${n} >/dev/null 2>&1`);
}

export default {
	// --- status (app polls this for the dashboard VPN tile) ----------------
	// The closed .so tracked a live peer; here we read `wg show` for the GL wg
	// client interface (`wgclient`) and report connected only on a recent
	// handshake. Degrades to status:0 (disconnected) when wg / the iface is absent.
	get_status: function(params, ctx) {
		let c = ctx.uci.cursor();
		// which saved config is active (last set)?
		let group_id = c.get(STORE, "state", "group_id") ?? "";
		let peer_id  = c.get(STORE, "state", "peer_id") ?? "";
		let p = (group_id != "" && peer_id != "") ? load_peer(c, group_id, peer_id) : null;
		let ep = split_endpoint(p ? p.end_point : "");

		// live tunnel state from `wg show <if> dump` (peer line has handshake/rx/tx)
		let dump = ctx.popen(`wg show ${WGIF} dump 2>/dev/null`);
		let status = 0, rx = 0, tx = 0;
		if (dump && length(dump)) {
			for (let line in split(dump, "\n")) {
				let f = split(line, "\t");
				if (length(f) < 8) continue;         // skip the interface (first) line
				let hs = int(f[4]);
				rx += int(f[5]); tx += int(f[6]);
				if (hs > 0 && (time() - hs) < 180) status = 1;   // recent handshake == up
			}
		}
		let s = ctx.ubus.call(`network.interface.${WGIF}`, "status") ?? {};
		let v4 = (s["ipv4-address"] && s["ipv4-address"][0]) ? s["ipv4-address"][0].address : "";
		return {
			status: status,                         // 0 down, 1 connected
			code: 0,
			name: p ? (p.name ?? "") : "",
			group_id: (group_id != "") ? int(group_id) : 0,
			peer_id: (peer_id != "") ? int(peer_id) : 0,
			rx_bytes: rx, tx_bytes: tx, rx: 0, tx: 0,
			domain: ep.host, port: (ep.port != "") ? int(ep.port) : 0,
			listen: 0, log: "",
			ipaddr: v4,
			main_server: ep.host, proxy: false
		};
	},

	// Import a WireGuard tunnel (a parsed .conf from the app). Stores it under a group
	// and returns the new peer_id. Fields mirror the app model: name, private_key,
	// public_key, preshared_key, end_point, allowed_ips, dns, address_v4/v6, mtu,
	// persistent_keepalive, listen_port, group_id.
	add_config: function(params, ctx) {
		let E = ctx.util.E;
		if (type(params) != "object" || !is_wg_key(params.private_key) || !is_wg_key(params.public_key))
			return E.INVALID_PARAMS;
		let c = ctx.uci.cursor();
		ensure_store(c);
		// group: reuse the caller's group_id or a default manual-import group (1)
		let group_id = `${params.group_id ?? 1}`;
		let gname = params.group_name ?? "Manual";
		let has_group = false;
		c.foreach(STORE, "group", function(s) { if (s.group_id == group_id) has_group = true; });
		if (!has_group) {
			let gid = c.add(STORE, "group");
			c.set(STORE, gid, "group_id", group_id);
			c.set(STORE, gid, "group_name", gname);
			c.set(STORE, gid, "group_type", "0");     // 0 = manual import
		}
		// next peer_id within the group
		let maxid = 0;
		c.foreach(STORE, "peers", function(s) {
			if (s.group_id == group_id && int(s.peer_id) > maxid) maxid = int(s.peer_id);
		});
		let peer_id = `${maxid + 1}`;
		let pid = c.add(STORE, "peers");
		let set = function(k, v) { if (v != null && v != "") c.set(STORE, pid, k, `${v}`); };
		set("group_id", group_id); set("peer_id", peer_id);
		set("name", params.name ?? `wg-${peer_id}`);
		set("private_key", params.private_key); set("public_key", params.public_key);
		set("preshared_key", params.preshared_key);
		set("end_point", params.end_point); set("allowed_ips", params.allowed_ips);
		set("dns", params.dns);
		set("address_v4", params.address_v4 ?? params.address);
		set("address_v6", params.address_v6);
		set("mtu", params.mtu); set("persistent_keepalive", params.persistent_keepalive);
		set("listen_port", params.listen_port);
		c.commit(STORE);
		ctx.popen("sync 2>/dev/null");
		return { group_id: int(group_id), peer_id: int(peer_id) };
	},

	// List saved groups (the app's WgGroupsM: {groups:[{group_id,group_name,...}]}).
	get_group_list: function(params, ctx) {
		let c = ctx.uci.cursor();
		let groups = [];
		c.foreach(STORE, "group", function(s) {
			let gid = s.group_id ?? "0";
			let n = 0;
			c.foreach(STORE, "peers", function(p) { if (p.group_id == gid) n++; });
			push(groups, {
				group_id: int(gid),
				group_name: s.group_name ?? "",
				group_type: int(s.group_type ?? "0"),
				peer_count: n,
				auth_type: int(s.auth_type ?? "0"),
				procedure: int(s.procedure ?? "0"),
				username: s.username ?? "",
				password: s.password ?? "",
				show: true
			});
		});
		return { groups: groups };
	},

	// List saved peer configs, optionally filtered by group_id (WgClientsM shape).
	get_config_list: function(params, ctx) {
		let c = ctx.uci.cursor();
		let filter = (params && params.group_id != null) ? `${params.group_id}` : null;
		let peers = [];
		c.foreach(STORE, "peers", function(s) {
			if (filter != null && s.group_id != filter) return;
			push(peers, {
				group_id: int(s.group_id ?? "0"), peer_id: int(s.peer_id ?? "0"),
				name: s.name ?? "", private_key: s.private_key ?? "",
				public_key: s.public_key ?? "", preshared_key: s.preshared_key ?? "",
				end_point: s.end_point ?? "", allowed_ips: s.allowed_ips ?? "",
				dns: s.dns ?? "", address_v4: s.address_v4 ?? "", address_v6: s.address_v6 ?? "",
				address: s.address_v4 ?? "", mtu: s.mtu ?? "",
				persistent_keepalive: s.persistent_keepalive ?? "", listen_port: s.listen_port ?? "",
				auth_type: int(s.auth_type ?? "0"), location: s.location ?? ""
			});
		});
		return { peers: peers };
	},

	// Connect: activate a saved tunnel (bring it up) and record the active group/peer.
	// This is the method the app's WireGuard page calls on the connect toggle.
	start: function(params, ctx) {
		let E = ctx.util.E;
		let c = ctx.uci.cursor();
		let group_id = `${params.group_id ?? ""}`, peer_id = `${params.peer_id ?? ""}`;
		if (group_id == "" || peer_id == "") return E.INVALID_PARAMS;
		let p = load_peer(c, group_id, peer_id);
		if (!p) return { err_code: -1, err_msg: "config not found" };
		apply_tunnel(ctx, p);
		ensure_store(c);
		c.set(STORE, "state", "state");
		c.set(STORE, "state", "group_id", group_id);
		c.set(STORE, "state", "peer_id", peer_id);
		c.commit(STORE);
		return {};
	},

	// Alias kept for callers that use set_config to activate a tunnel.
	set_config: function(params, ctx) { return this.start(params, ctx); },

	// Create/update/remove a group (the app groups peers by provider/import batch).
	add_group: function(params, ctx) {
		let c = ctx.uci.cursor();
		ensure_store(c);
		let maxg = 0;
		c.foreach(STORE, "group", function(s) { if (int(s.group_id) > maxg) maxg = int(s.group_id); });
		let gid = `${(params && params.group_id) ? int(params.group_id) : maxg + 1}`;
		let sec = c.add(STORE, "group");
		c.set(STORE, sec, "group_id", gid);
		c.set(STORE, sec, "group_name", (params && params.group_name) ? params.group_name : `Group ${gid}`);
		c.set(STORE, sec, "group_type", `${(params && params.group_type != null) ? int(params.group_type) : 0}`);
		c.commit(STORE);
		return { group_id: int(gid) };
	},

	set_group: function(params, ctx) {
		let c = ctx.uci.cursor();
		let gid = `${params.group_id ?? ""}`;
		if (gid == "") return ctx.util.E.INVALID_PARAMS;
		let sec;
		c.foreach(STORE, "group", function(s) { if (s.group_id == gid) sec = s[".name"]; });
		if (!sec) return { err_code: -1, err_msg: "group not found" };
		if (params.group_name != null) c.set(STORE, sec, "group_name", params.group_name);
		if (params.group_type != null) c.set(STORE, sec, "group_type", `${int(params.group_type)}`);
		c.commit(STORE);
		return {};
	},

	remove_group: function(params, ctx) {
		let c = ctx.uci.cursor();
		let gid = `${params.group_id ?? ""}`;
		if (gid == "") return ctx.util.E.INVALID_PARAMS;
		if (c.get(STORE, "state", "group_id") == gid) down_tunnel(ctx);
		let gsec, psecs = [];
		c.foreach(STORE, "group", function(s) { if (s.group_id == gid) gsec = s[".name"]; });
		c.foreach(STORE, "peers", function(s) { if (s.group_id == gid) push(psecs, s[".name"]); });
		for (let p in psecs) c.delete(STORE, p);
		if (gsec) c.delete(STORE, gsec);
		c.commit(STORE);
		return {};
	},

	// Stop the active tunnel (LAN traffic falls back to the physical/repeater uplink).
	stop: function(params, ctx) {
		let c = ctx.uci.cursor();
		down_tunnel(ctx);
		if (fs.access(`/etc/config/${STORE}`)) {
			c.delete(STORE, "state", "group_id");
			c.delete(STORE, "state", "peer_id");
			c.commit(STORE);
		}
		return {};
	},

	// Remove a saved config (and stop it if active).
	remove_config: function(params, ctx) {
		let E = ctx.util.E;
		let c = ctx.uci.cursor();
		let group_id = `${params.group_id ?? ""}`, peer_id = `${params.peer_id ?? ""}`;
		if (group_id == "" || peer_id == "") return E.INVALID_PARAMS;
		if (c.get(STORE, "state", "group_id") == group_id && c.get(STORE, "state", "peer_id") == peer_id)
			down_tunnel(ctx);
		let target;
		c.foreach(STORE, "peers", function(s) {
			if (s.group_id == group_id && s.peer_id == peer_id) target = s[".name"];
		});
		if (target) { c.delete(STORE, target); c.commit(STORE); }
		return {};
	},

	// --- provider-free core (reimplemented) --------------------------------

	gen_key: function(params, ctx) {
		params ??= {};

		// Need the wg binary; without it we degrade to an error, never crash.
		if (!ctx.popen("command -v wg 2>/dev/null")) {
			return { err_code: -1, err_msg: "wg binary not available" };
		}

		let priv;
		if (params.private_key != null && params.private_key != "") {
			// Caller-supplied key: validate length/format before shelling out.
			if (!is_wg_key(params.private_key)) {
				return { err_code: 20001207, err_msg: "Key is not the correct length or format" };
			}
			priv = params.private_key;
		} else {
			priv = ctx.popen("wg genkey 2>/dev/null");
		}

		if (!priv || !length(priv)) {
			return { err_code: -1, err_msg: "failed to generate private key" };
		}

		// base64 keys carry no shell metacharacters -> safe inside single quotes.
		let pub = ctx.popen("echo '" + priv + "' | wg pubkey 2>/dev/null");
		if (!pub || !length(pub)) {
			return { err_code: 20001207, err_msg: "Key is not the correct length or format" };
		}

		return { public_key: pub, private_key: priv };
	},

	get_max_client: function(params, ctx) {
		let mem_total = 0;
		let mem_avail = 0;

		let info = ctx.ubus.call("system", "info") ?? {};
		let mem = info.memory ?? {};
		if (type(mem) == "object") {
			mem_total = mem.total ?? 0;
			mem_avail = (mem.available != null)
				? mem.available
				: ((mem.free ?? 0) + (mem.buffered ?? 0));
		}

		// oui.fs.statvfs("/") 2nd return (available bytes) -> `df -k /`. Match the
		// used/available/use% numeric run so a wrapped df line still parses.
		let flash_avail = 0;
		let df = ctx.popen("df -k / 2>/dev/null");
		if (df) {
			let m = match(df, /([0-9]+)[ \t]+([0-9]+)[ \t]+([0-9]+)[ \t]+[0-9]+%/);
			if (m) flash_avail = int(m[3]) * 1024;
		}
		let flash_total = 0;   // DEGRADE: /proc/gl-hw-info/flash_size absent on mainline

		mem_avail = mem_avail - (mem_total * 0.05);
		flash_avail = flash_avail - (flash_total * 0.05);

		let max_count;
		if (flash_avail < mem_avail) {
			max_count = flash_avail / 700;
		} else {
			max_count = mem_avail / 700;
		}

		// if available memory is too small, limit again
		if (max_count < 500) {
			max_count = max_count / 2;
		}

		let mc = int(max_count);   // int() truncates toward zero == floor for >=0
		if (mc < 0) mc = 0;

		return { max_client: mc };
	},

	// --- third-party VPN providers (stubbed: closed cloud clients) ----------

	get_provider_server_list: function(params, ctx) {
		return unsupported_provider();
	},

	generate_provider_config: function(params, ctx) {
		return unsupported_provider();
	},

	update_provider_config: function(params, ctx) {
		return unsupported_provider();
	},

	create_provider_account: function(params, ctx) {
		return unsupported_provider();
	},

	get_provider_user_info: function(params, ctx) {
		return unsupported_provider();
	},

	start_random_client: function(params, ctx) {
		return unsupported_provider();
	},

	init_provider_group: function(params, ctx) {
		return unsupported_provider();
	},

	// --- ExpressVPN GL-Cloud co-branded wizard (stubbed: disabled) ----------

	expressvpn_get_status: function(params, ctx) {
		return unsupported_xv();
	},

	expressvpn_check_eligibility: function(params, ctx) {
		return unsupported_xv();
	},

	expressvpn_accept_agreement: function(params, ctx) {
		return unsupported_xv();
	},

	expressvpn_request_checkout: function(params, ctx) {
		return unsupported_xv();
	},

	expressvpn_poll_order_activation: function(params, ctx) {
		return unsupported_xv();
	},

	expressvpn_start_login: function(params, ctx) {
		return unsupported_xv();
	},

	expressvpn_poll_login: function(params, ctx) {
		return unsupported_xv();
	},

	expressvpn_cancel_login: function(params, ctx) {
		return unsupported_xv();
	},

	expressvpn_fetch_srt: function(params, ctx) {
		return unsupported_xv();
	},

	expressvpn_get_subscriptions: function(params, ctx) {
		return unsupported_xv();
	}
};
