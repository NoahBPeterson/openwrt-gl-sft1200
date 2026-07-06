// gl-ucode object: flow_statistics  (port of decompiled-spec/rpc/flow_statistics.lua)
// Per-app / per-client traffic charts. The original was backed by GL's netifyd DPI
// daemon writing an sqlite DB (/tmp/traffic_data.db) plus lsqlite3 in the RPC layer.
// Neither netifyd nor an sqlite binding exists here, so:
//   DROP: lsqlite3/sqlite entirely, the netify app-metadata, and the MediaTek
//         hw-offload disable (gl-session get/set_netnat_config), the netifyd start/
//         stop and the nft/iptables NFQUEUE plumbing (all netifyd-specific).
//   KEEP: the gl_dpi_flow_statistics uci as our OWN store for the rule so
//         set_statistics_rule/get_statistics_rule round-trip faithfully.
//   DEGRADE (reads): with no netify DB there is no per-app classification. The read
//         methods return {enable:false} when the rule is off, else empty stat
//         structures with the SAME field names/shapes as the Lua. The ONLY real data
//         we can offer is the per-CLIENT (not per-app) "all traffic" view
//         (application_id == "-1"), sourced best-effort from nlbwmon (`nlbw -c csv`)
//         if installed, else empty. We never invent per-app data.
'use strict';

const fs = require('fs');

const TIME_WINDOWS = { hour: 3600, day: 86400, week: 604800 };

// mirror of the Lua get_relative_time()
function get_relative_time(timestamp) {
	let diff = time() - timestamp;
	if (diff < 60) return "just now";
	if (diff < 3600) return sprintf("%d minutes ago", int(diff / 60));
	if (diff < 86400) return sprintf("%d hours ago", int(diff / 3600));
	return sprintf("%d days ago", int(diff / 86400));
}

// resolve the configured "time" window -> seconds (default 3600 like the hour window)
function period_seconds_for(t) {
	return TIME_WINDOWS[t] ?? 3600;
}

// mac -> friendly name, from dhcp leases + gl-client aliases (replaces the gl_clients
// daemon socket + gl-client foreach in the Lua). alias wins over lease hostname.
function mac_name_map(ctx) {
	let names = {};
	let raw = fs.readfile("/tmp/dhcp.leases");
	if (type(raw) == "string" && length(raw)) {
		for (let line in split(trim(raw), "\n")) {
			let f = split(line, " ");        // <expiry> <mac> <ip> <name> <id>
			if (length(f) >= 4 && f[3] != "*")
				names[uc(f[1])] = f[3];
		}
	}
	let c = ctx.uci.cursor();
	c.foreach("gl-client", "client", function(s) {
		if (s.mac && s.alias)
			names[uc(s.mac)] = s.alias;
	});
	return names;
}

// Best-effort per-MAC totals from nlbwmon. Returns an array of
// {mac, download, upload, packets, conns} or [] if nlbwmon is absent / unparsable.
// DEGRADE: nlbwmon accounts cumulative lifetime totals (not the requested time
// window) and has no per-app breakdown, so this only feeds the all-traffic view.
function nlbw_by_mac(ctx) {
	let out = [];
	let raw = ctx.popen("nlbw -c csv -g mac -o -rx_bytes 2>/dev/null");
	if (type(raw) != "string" || length(raw) == 0)
		return out;
	let lines = split(trim(raw), "\n");
	if (length(lines) < 2)
		return out;

	// header-driven column lookup (robust to column ordering)
	let hdr = split(lines[0], ",");
	let col = {};
	for (let i = 0; i < length(hdr); i++)
		col[lc(trim(hdr[i]))] = i;
	let i_mac = col["mac"];
	let i_rx = col["rx_bytes"];
	let i_tx = col["tx_bytes"];
	let i_rxp = col["rx_pkts"];
	let i_txp = col["tx_pkts"];
	let i_conn = col["conns"];
	if (i_mac == null || i_rx == null || i_tx == null)
		return out;   // not the CSV we expect -> degrade to empty

	for (let li = 1; li < length(lines); li++) {
		let f = split(lines[li], ",");
		if (length(f) <= i_mac)
			continue;
		let mac = uc(trim(f[i_mac]));
		if (length(mac) != 17)
			continue;
		let rxp = (i_rxp != null && length(f) > i_rxp) ? int(f[i_rxp]) : 0;
		let txp = (i_txp != null && length(f) > i_txp) ? int(f[i_txp]) : 0;
		push(out, {
			mac: mac,
			download: int(f[i_rx]),
			upload: int(f[i_tx]),
			packets: rxp + txp,
			conns: (i_conn != null && length(f) > i_conn) ? int(f[i_conn]) : 0
		});
	}
	return out;
}

// The per-client "all traffic" view (Lua get_all_traffic), filled from nlbwmon when
// available. time_series is left empty (no windowed sampling without netifyd).
function all_traffic(ctx, period_seconds) {
	let now = time();
	let result = {
		identifier: "-1",
		period_seconds: period_seconds,
		max_bytes: "0",
		total_download: 0,
		total_upload: 0,
		mac_addresses: {},
		time_series: [],
		metadata: { start_time: now - period_seconds, end_time: now },
		logo: "",
		desc: "This is the total traffic passing through the router after the function is enabled.",
		url: "",
		application_id: "-1",
		application_name: "all_traffic",
		label: "All traffic"
	};
	let names = mac_name_map(ctx);
	for (let e in nlbw_by_mac(ctx)) {
		let total = e.download + e.upload;
		result.total_download += e.download;
		result.total_upload += e.upload;
		let bigger = (e.download > e.upload) ? e.download : e.upload;
		if (int(result.max_bytes) < bigger)
			result.max_bytes = sprintf("%d", bigger);
		result.mac_addresses[e.mac] = {
			hostname: names[e.mac] ?? "Unknown",
			download: e.download,
			upload: e.upload,
			total: total,
			packets: e.packets,
			record_count: e.conns,
			last_active_time: now,
			last_active_relative: get_relative_time(now)
		};
	}
	return result;
}

// empty per-app "specific app" view (Lua get_specific_app_traffic shape). No netify
// DB and no per-app classification -> zeros, no mac_addresses.
function empty_specific_app(app_id, period_seconds) {
	let now = time();
	return {
		identifier: app_id,
		period_seconds: period_seconds,
		max_bytes: "0",
		total_download: 0,
		total_upload: 0,
		mac_addresses: {},
		time_series: [],
		metadata: { start_time: now - period_seconds, end_time: now },
		application_id: app_id,
		application_name: "Unknown",
		application_block: false,
		label: "Unclassified",
		logo: "",
		desc: "",
		url: ""
	};
}

export default {
	// Persist the charting rule. Store schema (gl_dpi_flow_statistics.global.*) is kept
	// identical to GL so the app round-trips. DEGRADE: the MTK hw-offload disable
	// (gl-session get/set_netnat_config), the netifyd start/stop and the nft/iptables
	// NFQUEUE plumbing are all netifyd-specific and dropped (no netifyd here).
	set_statistics_rule: function(args, ctx) {
		let c = ctx.uci.cursor();
		if (!c.get("gl_dpi_flow_statistics", "global"))
			c.set("gl_dpi_flow_statistics", "global", "global");
		if (args.enable != null)
			c.set("gl_dpi_flow_statistics", "global", "enable", args.enable ? "1" : "0");
		if (args.time != null && (args.time == "hour" || args.time == "day" || args.time == "week"))
			c.set("gl_dpi_flow_statistics", "global", "time", args.time);
		if (args.type != null && (args.type == "app" || args.type == "category"))
			c.set("gl_dpi_flow_statistics", "global", "type", args.type);
		c.commit("gl_dpi_flow_statistics");
		ctx.popen("sync");
		return {};
	},

	get_statistics_rule: function(args, ctx) {
		let c = ctx.uci.cursor();
		return {
			enable: c.get("gl_dpi_flow_statistics", "global", "enable") == "1",
			time: c.get("gl_dpi_flow_statistics", "global", "time"),
			type: c.get("gl_dpi_flow_statistics", "global", "type")
		};
	},

	// DEGRADE: no netify DB -> when the rule is on we return an empty stat structure
	// (same shape as get_app_traffic_stats / get_category_traffic_stats); when off,
	// {enable:false}.
	get_flow_statistics: function(args, ctx) {
		let c = ctx.uci.cursor();
		let t = args.time ?? c.get("gl_dpi_flow_statistics", "global", "time") ?? "hour";
		let ty = args.type ?? c.get("gl_dpi_flow_statistics", "global", "type") ?? "app";
		if (c.get("gl_dpi_flow_statistics", "global", "enable") != "1")
			return { enable: false };
		if (ty == "app")
			return {
				time: t,
				applications: [{
					application_id: "-1",
					application_name: "all_traffic",
					download: 0,
					upload: 0,
					total: 0,
					label: "All traffic",
					icon: ""
				}]
			};
		return { time: t, category: [] };
	},

	// DEGRADE: per-app view needs netifyd. The all-traffic (application_id "-1") view
	// is per-CLIENT and sourced from nlbwmon when present; specific apps return an
	// empty specific-app structure.
	get_app_flow_statistics: function(args, ctx) {
		let c = ctx.uci.cursor();
		let t = args.time ?? c.get("gl_dpi_flow_statistics", "global", "time") ?? "hour";
		let ty = args.type ?? c.get("gl_dpi_flow_statistics", "global", "type") ?? "app";
		if (c.get("gl_dpi_flow_statistics", "global", "enable") != "1")
			return { enable: false };
		if (ty != "app")
			return {};
		let secs = period_seconds_for(t);
		if (args.application_id == "-1")
			return all_traffic(ctx, secs);
		return empty_specific_app(args.application_id, secs);
	},

	// DEGRADE: top-app ranking needs netifyd's per-app classification -> empty list.
	get_top_app_flow_statistics: function(args, ctx) {
		let c = ctx.uci.cursor();
		let t = args.time ?? c.get("gl_dpi_flow_statistics", "global", "time") ?? "hour";
		let ty = args.type ?? c.get("gl_dpi_flow_statistics", "global", "type") ?? "app";
		if (c.get("gl_dpi_flow_statistics", "global", "enable") != "1")
			return { enable: false };
		if (ty != "app")
			return {};
		return {
			period_seconds: period_seconds_for(t),
			max_bytes: "0",
			top_apps: []
		};
	},

	// DEGRADE: cleared the netify sqlite table + restarted gl_dpi_flow_statistics.
	// No DB and no daemon here -> no-op.
	clear_statistics: function(args, ctx) {
		return {};
	}
};
