// gl-ucode object: clients  (port of decompiled-spec/rpc/clients.lua)
// get_list/get_status build the "connected devices" view from mainline sources
// (DHCP leases + neighbour table) instead of GL's gl_clients daemon.
// The naming/blocking/limit methods keep GL's uci schemas as their OWN stores and
// re-implement enforcement over mainline (nft / dhcp uci), degrading every
// gl_clients daemon socket + ubus("gl-clients",...) call. See per-method DEGRADE notes.
'use strict';

const fs = require('fs');

const MAC_RE = /^[0-9a-fA-F]{2}(:[0-9a-fA-F]{2}){5}$/;

function leases() {
	let out = {}, raw = fs.readfile("/tmp/dhcp.leases");
	if (!raw) return out;
	for (let line in split(trim(raw), "\n")) {
		let f = split(line, " ");          // <expiry> <mac> <ip> <name> <id>
		if (length(f) >= 4) out[lc(f[1])] = { ip: f[2], name: (f[3] != "*") ? f[3] : "" };
	}
	return out;
}

// uci list options come back as an array; a single value as a string; missing as
// null. Normalise everything to an array.
function as_list(v) {
	if (type(v) == "array") return v;
	if (type(v) == "string" && length(v) > 0) return [v];
	return [];
}

// Set of lowercase MACs the neighbour table reports as currently/recently up.
function online_macs(ctx) {
	let set = {};
	let neigh = ctx.popen("ip neigh show 2>/dev/null");
	if (type(neigh) != "string") return set;
	for (let line in split(neigh, "\n")) {
		let m = match(line, /^([0-9.]+) dev (\S+) lladdr ([0-9a-fA-F:]{17})/);
		if (!m || substr(m[2], 0, 3) != "br-") continue;
		let sm = match(line, /(REACHABLE|STALE|DELAY|PROBE|PERMANENT|NOARP|FAILED|INCOMPLETE|NONE)[[:space:]]*$/);
		let st = sm ? sm[1] : "";
		if (st == "REACHABLE" || st == "STALE" || st == "DELAY" || st == "PROBE" || st == "PERMANENT")
			set[lc(m[3])] = true;
	}
	return set;
}

// DEGRADE: replaces GL's /etc/init.d/firewall reload of the `gl_block` ruleset
// (that translator is absent on mainline). Rebuilds a self-managed `inet gl_block`
// nft table from the `gl_block.block_mac.mac` store — drop LAN-origin traffic from
// listed MACs. Idempotent. NOTE: this overlaps black_white_list.uc (blacklist mode);
// they use separate nft tables so both can coexist.
function apply_block(ctx) {
	let c = ctx.uci.cursor();
	let macs = as_list(c.get("gl_block", "block_mac", "mac"));
	let clean = [];
	for (let m in macs) if (type(m) == "string" && match(m, MAC_RE)) push(clean, lc(m));

	// always start from a clean slate (delete-if-exists without aborting)
	ctx.popen("nft add table inet gl_block 2>/dev/null; nft delete table inet gl_block 2>/dev/null");
	if (length(clean) == 0)
		return true;   // nothing to enforce -> table left absent

	let ruleset =
		"table inet gl_block {\n" +
		"\tset blocklist {\n" +
		"\t\ttype ether_addr\n" +
		"\t\telements = { " + join(", ", clean) + " }\n" +
		"\t}\n" +
		"\tchain fwd {\n" +
		"\t\ttype filter hook forward priority filter; policy accept;\n" +
		"\t\tiifname \"br-lan\" ether saddr @blocklist drop\n" +
		"\t}\n" +
		"\tchain inp {\n" +
		"\t\ttype filter hook input priority filter; policy accept;\n" +
		"\t\tiifname \"br-lan\" ether saddr @blocklist drop\n" +
		"\t}\n" +
		"}\n";

	fs.writefile("/tmp/gl-block.nft", ruleset);
	ctx.popen("nft -f /tmp/gl-block.nft 2>/dev/null");
	return true;
}

// Map of lowercase MAC -> { band, iface, dl, ul } for every station currently
// associated to a local AP. GL's gl-clients daemon derived a client's connection
// type + per-client byte counters from its wifi station table; that daemon is
// absent on mainline, so we rebuild the same facts from `iw`. A MAC present here
// is wireless (with its band); a leased/neighbour MAC absent here is wired.
// Byte semantics (AP perspective): rx bytes = received FROM station = client
// upload; tx bytes = sent TO station = client download.
function wifi_map(ctx) {
	let map = {};
	// One shell pass: for each AP-mode iface, emit "<band> <mac> <rx> <tx>" lines.
	let sh =
		"for i in $(iw dev 2>/dev/null | awk '/Interface/{print $2}'); do " +
		"iw dev $i info 2>/dev/null | grep -q 'type AP' || continue; " +
		"ch=$(iw dev $i info 2>/dev/null | awk '/channel/{print $2; exit}'); " +
		"b=5G; [ \"$ch\" -le 14 ] 2>/dev/null && b=2.4G; " +
		"iw dev $i station dump 2>/dev/null | awk -v b=$b '" +
		"/^Station/{m=$2} /rx bytes:/{r=$3} /tx bytes:/{print b, m, r, $3}'; " +
		"done";
	let out = ctx.popen(sh);
	if (type(out) != "string") return map;
	for (let line in split(out, "\n")) {
		let f = split(trim(line), " ");
		if (length(f) < 4) continue;
		map[lc(f[1])] = { band: f[0], ul: int(f[2]), dl: int(f[3]) };
	}
	return map;
}

// Add GL's connection-type + traffic fields to a client record from its wifi
// station entry (or mark it wired when absent). `type` uses GL's exact
// get_connect_type enum (from gl_clients_update): 2.4G=0, 5G=1, cable=2,
// 2.4G_Guest=3, 5G_Guest=4, Unknown=5, 6G=11, 6G_Guest=12, tethering=13.
// total_rx/total_tx = cumulative download/upload bytes; rx/tx (live speed) need
// interval sampling we don't do in a getter, so report 0.
function band_type(band) {
	if (band == "2.4G") return 0;
	if (band == "5G")   return 1;
	if (band == "6G")   return 11;
	return 1;                        // default any other wireless band to 5G
}
function enrich(rec, w) {
	if (w) {
		rec.type = band_type(w.band);
		rec.band = w.band;
		rec.iface = w.band;
		rec.total_rx = w.dl; rec.total_tx = w.ul;
	} else {
		rec.type = 2;                // cable
		rec.band = "cable";
		rec.iface = "cable";
		rec.total_rx = 0; rec.total_tx = 0;
	}
	rec.rx = 0; rec.tx = 0;         // instantaneous speed (see note)
	return rec;
}

export default {
	get_list: function(args, ctx) {
		let lz = leases();
		let wm = wifi_map(ctx);
		let by_mac = {};
		// Neighbours on the LAN bridges only. `ip neigh show` spans every interface,
		// including the WAN uplink — which would list the upstream gateway(s)
		// (e.g. 192.168.2.1) as "clients". Filter to dev br-* so only LAN-side hosts
		// (lan/guest/iot bridges) appear. State REACHABLE/STALE/DELAY/PROBE/PERMANENT
		// == a live ARP entry == connected; FAILED/INCOMPLETE == gone.
		let neigh = ctx.popen("ip neigh show 2>/dev/null");
		if (type(neigh) != "string") neigh = "";   // narrow string|null -> string
		for (let line in split(neigh, "\n")) {
			let m = match(line, /^([0-9.]+) dev (\S+) lladdr ([0-9a-fA-F:]{17})/);
			if (!m || length(m) < 4) continue;
			let dev = m[2];
			if (substr(dev, 0, 3) != "br-") continue;   // LAN bridges only
			let mac = lc(m[3]);
			// The NUD state is the trailing keyword; this kernel prints
			// "used x/x/x probes N <STATE>" after the lladdr, so grab it explicitly.
			let sm = match(line, /(REACHABLE|STALE|DELAY|PROBE|PERMANENT|NOARP|FAILED|INCOMPLETE|NONE)[[:space:]]*$/);
			let st = sm ? sm[1] : "";
			let up = (st == "REACHABLE" || st == "STALE" || st == "DELAY" || st == "PROBE" || st == "PERMANENT");
			// a host can have entries on several bridges; keep it online if any is up
			if (by_mac[mac] && by_mac[mac].online) up = true;
			by_mac[mac] = enrich({
				mac: mac,
				ip: m[1],
				name: (lz[mac] && lz[mac].name) ? lz[mac].name : "",
				online: up
			}, wm[mac]);
		}
		// include leased-but-quiet hosts
		for (let mac in lz)
			if (!by_mac[mac])
				by_mac[mac] = enrich({ mac: mac, ip: lz[mac].ip, name: lz[mac].name, online: false }, wm[mac]);
		let clients = [];
		for (let mac in by_mac) push(clients, by_mac[mac]);
		return { clients: clients };
	},

	get_status: function(args, ctx) {
		let n = 0;
		let r = this.get_list(args, ctx);
		for (let c in r.clients) if (c.online) n++;
		if (type(r.clients) != "string" && type(r.clients) != "array" && type(r.clients) != "object")
			return;
		return { online: n, total: length(r.clients) };
	},

	// Block/unblock a client by MAC. Store schema (gl_block.block_mac.mac[]) is kept
	// identical to GL; enforcement is degraded to nft via apply_block().
	block_client: function(args, ctx) {
		if (type(args.mac) != "string")
			return ctx.util.E.INVALID_PARAMS;
		let mac = uc(args.mac);
		let block = args.block;
		let c = ctx.uci.cursor();
		c.set("gl_block", "block_mac", "access");
		c.set("gl_block", "block_mac", "name", "mac_block");
		let blocked = as_list(c.get("gl_block", "block_mac", "mac"));
		let next = [];
		for (let m in blocked) if (uc(m) != mac) push(next, m);
		if (block) push(next, mac);
		if (length(next) == 0) c.delete("gl_block", "block_mac", "mac");
		else c.set("gl_block", "block_mac", "mac", next);
		c.commit("gl_block");
		ctx.popen("sync");
		// DEGRADE: ngx.pipe.spawn("/etc/init.d/firewall reload") -> apply_block() below.
		apply_block(ctx);
		return {};
	},

	// Set a client's friendly name (alias) and/or class. Store schema (gl-client
	// `client` sections + dhcp `host.tag` mirror) is kept identical to GL.
	set_info: function(args, ctx) {
		if (!args.mac)
			return {};
		// require alias or class; reject aliases containing '"' or ',' (as the original did)
		if (!(args.alias || args.class))
			return {};
		if (args.alias && (index(args.alias, "\"") >= 0 || index(args.alias, ",") >= 0))
			return {};

		let c = ctx.uci.cursor();
		let target = uc(args.mac);
		let found = false;
		c.foreach("gl-client", "client", function(s) {
			if (s.mac && uc(s.mac) == target) {
				if (args.alias) {
					c.set("gl-client", s[".name"], "alias", args.alias);
					// mirror the alias onto the dhcp host tag (mainline dhcp config)
					c.foreach("dhcp", "host", function(host) {
						if (host.mac && uc(host.mac) == target)
							c.set("dhcp", host[".name"], "tag", args.alias);
					});
				}
				if (args.class)
					c.set("gl-client", s[".name"], "class", args.class);
				found = true;
			}
		});
		if (!found) {
			let sid = c.add("gl-client", "client");   // commit creates the config file if absent
			c.set("gl-client", sid, "mac", target);
			if (args.alias) c.set("gl-client", sid, "alias", args.alias);
			if (args.class) c.set("gl-client", sid, "class", args.class);
		}
		c.commit("gl-client");
		c.commit("dhcp");
		ctx.popen("sync");
		return {};
	},

	// Forget an offline client (or, for FF:FF:FF:FF:FF:FF, all offline clients).
	// DEGRADE: GL cached offline clients in the gl_clients daemon and pruned them
	// via ubus("gl-clients","remove_offline"). Our list is derived live from
	// /tmp/dhcp.leases, so "forgetting" = drop the host's lease line (so it stops
	// appearing) plus tidy any qos limit / gl-client alias entries for that MAC.
	// (dnsmasq keeps its own in-memory copy until it reloads/expires.)
	remove_offline: function(args, ctx) {
		let mac = args.mac;
		if (type(mac) != "string")
			return ctx.util.E.INVALID_PARAMS;

		let c = ctx.uci.cursor();
		// choose target MACs
		let targets = {};
		if (uc(mac) == "FF:FF:FF:FF:FF:FF") {
			let online = online_macs(ctx);
			let lz = leases();
			for (let m in lz) if (!online[m]) targets[m] = true;
		} else {
			targets[lc(mac)] = true;
		}

		// tidy per-client qos limit sections (reuse qos store; replaces gl_eqos del)
		if (fs.access("/etc/config/qos")) {
			for (let m in targets) {
				let sec;
				c.foreach("qos", "queue", function(s) {
					if (s.mac && lc(s.mac) == m) sec = s[".name"];
				});
				if (sec) c.delete("qos", sec);
			}
			c.commit("qos");
		}
		// tidy gl-client alias/class entries
		if (fs.access("/etc/config/gl-client")) {
			for (let m in targets) {
				let sec;
				c.foreach("gl-client", "client", function(s) {
					if (s.mac && lc(s.mac) == m) sec = s[".name"];
				});
				if (sec) c.delete("gl-client", sec);
			}
			c.commit("gl-client");
		}

		// rewrite the lease file without the forgotten hosts
		let raw = fs.readfile("/tmp/dhcp.leases");
		if (type(raw) == "string" && length(raw)) {
			let out = [];
			for (let line in split(trim(raw), "\n")) {
				let f = split(line, " ");
				if (length(f) >= 4 && targets[lc(f[1])]) continue;
				if (length(line)) push(out, line);
			}
			fs.writefile("/tmp/dhcp.leases", join("\n", out) + "\n");
		}
		ctx.popen("sync");
		return {};
	},

	// DEGRADE: GL cleared per-client traffic counters in the gl_clients daemon
	// (ubus("gl-clients","clean_traffic")). No mainline per-MAC byte accounting
	// exists, so there is nothing to reset -> no-op.
	clean_traffic: function(args, ctx) {
		return {};
	},

	// DEGRADE: aggregate per-client throughput came from the gl_clients daemon's
	// traffic accounting (absent). No mainline per-MAC rate counters -> zeros
	// (same field names/shape as the original fallback).
	get_speed: function(args, ctx) {
		return { speed_tx: 0, speed_rx: 0 };
	},

	// DEGRADE: WAN throughput came from the gl_clients daemon (absent). A faithful
	// value needs a sampling daemon (two /proc/net/dev reads over an interval);
	// getters must stay light, so return zeros (same shape as the original fallback).
	get_wan_speed: function(args, ctx) {
		return { speed_tx: 0, speed_rx: 0 };
	},

	// Toggle auto-removal of offline clients. Store schema (gl-client.@global[0]
	// .cache_offline) is kept identical to GL.
	set_cache_offline: function(args, ctx) {
		if (type(args.auto_remove_offline) != "bool")
			return { err_code: 200027002, err_msg: "invalid parameter" };
		let c = ctx.uci.cursor();
		if (c.get("gl-client", "@global[0]") != "global")
			c.add("gl-client", "global");
		c.set("gl-client", "@global[0]", "cache_offline", args.auto_remove_offline ? "0" : "1");
		c.commit("gl-client");
		ctx.popen("sync");
		// DEGRADE: ubus("gl-clients","modify_cache_offline") dropped. Our get_list is
		// live (leases+neigh) with no offline cache to prune, so this flag is
		// store-only; remove_offline() performs the actual forgetting on demand.
		return {};
	}
};
