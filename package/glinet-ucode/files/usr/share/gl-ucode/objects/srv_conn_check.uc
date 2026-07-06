// gl-ucode object: srv_conn_check  (port of decompiled-spec/rpc/srv_conn_check.lua)
// "Is my VPN server port reachable from the internet?" Opens a temporary WAN
// input-accept rule for the configured VPN server port, fetches the public IP,
// then asks the GL public scan endpoint to connect back to that port and reports
// verified / mismatch. Uses a lockfile so only one probe runs at a time.
//
// DEGRADE / DROP notes:
//  - eco.http.client / curl -> busybox `wget` (no curl on target).
//  - The upstream helper (/usr/bin/srv_conn_check, Lua-5.4 bytecode over eco.socket)
//    opened raw listen sockets and exchanged an *encrypted* probe (eco.crypto) with
//    the middle server. That handshake cannot be reproduced with wget alone, so the
//    scan is delegated to the public GL endpoint: we POST {public_ip,port,protocol}
//    and read the verified flags straight from its JSON reply. If the endpoint is
//    unreachable / unparseable we degrade to the natural VPN error shape.
//  - ngx.pipe.spawn("eco ...") subprocess -> in-process wget flow (run_check()).
//  - utils.call_rpc("network","get_available_gateway_ip") -> read ubus
//    network.interface.wan status directly (no cross-object calls allowed).
//  - glipv6.globals.enabled read if present, else false (GL config absent on mainline).
//  - iptables/ip6tables legacy branch kept only as a fallback; mainline uses fw4/nft.
'use strict';

const fs = require('fs');

const LOCK_FILE = "/var/run/srv_conn_check.lock";
const SCAN_ENDPOINT = "https://fw.gl-inet.com/vpn/connect/srv-conn-check";
const SCAN_FALLBACK = "https://gl-vpn-unicom.gl-vpn.com";

// Subset of gl.vpn_err_code actually used by this object.
const E = {
	PARAM_MISSING:                20001011,
	PARAM_INVALIED:               20001012,
	GET_CONF_FAIL:                20001015,
	SERVER_IS_RUNNING:            20001031,
	PORT_BE_USED:                 20001033,
	CONNECT_SERVER_FAIL:          20001214,
	OPERATE_TOO_FREQUENT:         20001220,
	EXTERNAL_SERVICE_UNAVAILABLE: 20001221
};
const EMSG = {
	"20001011": "parameter missing",
	"20001012": "invalid parameter",
	"20001015": "get config data failed",
	"20001031": "vpn server is running",
	"20001033": "the port is already occupied",
	"20001214": "get information from provider server failed",
	"20001220": "operation too frequent, please try again later",
	"20001221": "external service is unavailable"
};

function err_pack(code, msg) {
	return {
		err_code: code,
		err_msg: msg ?? EMSG[`${code}`] ?? "unknown error"
	};
}

// nil/empty -> CONNECT_SERVER_FAIL; otherwise map the helper's error text to a code
// exactly as the Lua map_bin_error did.
function map_bin_error(msg) {
	if (!msg || msg == "")
		return E.CONNECT_SERVER_FAIL;
	if (match(msg, /bind/) || match(msg, /Address already in use/))
		return E.PORT_BE_USED;
	if (match(msg, /Too Many requests/) || match(msg, /Try Again Later/))
		return E.OPERATE_TOO_FREQUENT;
	if (match(msg, /scan.*failed/) || match(msg, /service.*unavailable/) || match(msg, /Scan server request failed/))
		return E.EXTERNAL_SERVICE_UNAVAILABLE;
	return E.CONNECT_SERVER_FAIL;
}

// ---- lock -------------------------------------------------------------------
function acquire_lock(ctx) {
	if (fs.access(LOCK_FILE))
		return false;
	ctx.popen(`touch ${LOCK_FILE} 2>/dev/null`);
	return true;
}

function release_lock(ctx) {
	ctx.popen(`rm -f ${LOCK_FILE} 2>/dev/null`);
}

// ---- vpn config -------------------------------------------------------------
// Faithful port of get_vpn_config(): returns { port, protocol } or an int err code.
// Note the (odd but intentional) upstream semantics: the check is only allowed when
// the server is *configured but stopped* (disabled). A running server -> error.
function get_vpn_config(ctx, iface) {
	let c = ctx.uci.cursor();

	if (iface == "ovpnserver") {
		let disabled = c.get("network", "ovpnserver", "disabled");
		if (c.get("network", "ovpnserver") != null && (disabled == null || disabled != "1"))
			return E.SERVER_IS_RUNNING;
		let port_str = c.get("ovpnserver", "vpn", "port");
		let port = (port_str != null) ? int(port_str) : null;
		let proto = c.get("ovpnserver", "vpn", "proto");
		if (port == null)
			return E.GET_CONF_FAIL;
		return { port: port, protocol: proto ?? "udp" };
	}
	else if (iface == "wgserver") {
		let disabled = c.get("network", "wgserver", "disabled");
		if (c.get("network", "wgserver") != null && (disabled == null || disabled != "1"))
			return E.SERVER_IS_RUNNING;
		let port_str = c.get("wireguard_server", "main_server", "port");
		let port = (port_str != null) ? int(port_str) : null;
		if (port == null)
			return E.GET_CONF_FAIL;
		return { port: port, protocol: "udp" };
	}
	return E.PARAM_INVALIED;
}

// ---- firewall ---------------------------------------------------------------
function has_fw4(ctx) {
	let out = ctx.popen("which fw4 2>/dev/null");
	return (type(out) == "string" && trim(out) != "");
}

function ipv6_enabled(ctx) {
	// GL glipv6 config is absent on mainline -> default false.
	let c = ctx.uci.cursor();
	return (c.get("glipv6", "globals", "enabled") == "1");
}

function add_firewall_rule(ctx, port, protocol) {
	if (has_fw4(ctx)) {
		ctx.popen(sprintf("nft insert rule inet fw4 input_wan %s dport %d counter accept comment \\\"srv_conn_check_temp\\\" 2>/dev/null",
			protocol, port));
	} else {
		ctx.popen(sprintf("iptables -I zone_wan_input -p %s --dport %d -j ACCEPT -m comment --comment srv_conn_check_temp 2>/dev/null",
			protocol, port));
		if (ipv6_enabled(ctx))
			ctx.popen(sprintf("ip6tables -I zone_wan_input -p %s --dport %d -j ACCEPT -m comment --comment srv_conn_check_temp 2>/dev/null",
				protocol, port));
	}
}

function remove_firewall_rule(ctx, port, protocol) {
	if (has_fw4(ctx)) {
		ctx.popen("nft -a list chain inet fw4 input_wan 2>/dev/null | grep 'srv_conn_check_temp' | awk '{print $NF}' | xargs -r -n1 nft delete rule inet fw4 input_wan handle 2>/dev/null");
	} else {
		ctx.popen(sprintf("iptables -D zone_wan_input -p %s --dport %d -j ACCEPT -m comment --comment srv_conn_check_temp 2>/dev/null",
			protocol, port));
		if (ipv6_enabled(ctx))
			ctx.popen(sprintf("ip6tables -D zone_wan_input -p %s --dport %d -j ACCEPT -m comment --comment srv_conn_check_temp 2>/dev/null",
				protocol, port));
	}
}

// ---- wan / public ip --------------------------------------------------------
// Replacement for network.get_available_gateway_ip: read WAN status via ubus.
function get_network_gateway_ip(ctx) {
	let s = ctx.ubus.call("network.interface.wan", "status") ?? {};
	let ip = "";
	if (s["ipv4-address"] && s["ipv4-address"][0] && s["ipv4-address"][0].address)
		ip = s["ipv4-address"][0].address;
	let gw = "";
	if (s.route) {
		for (let r in s.route) {
			if (r.target == "0.0.0.0" && int(r.mask) == 0 && r.nexthop) {
				gw = r.nexthop;
				break;
			}
		}
	}
	return {
		gateway_ip: (type(gw) == "string") ? gw : "",
		internal_ip: (type(ip) == "string") ? ip : ""
	};
}

// Fetch the public address(es). busybox wget has no -4/-6 family flag, so we take
// whatever each host returns and classify it. Mirrors the helper's ip.me/ip.gs list.
function fetch_public_ips(ctx) {
	let v4 = "", v6 = "";
	for (let host in ["ip.me", "ip.gs", "ifconfig.me/ip"]) {
		if (v4 != "" && v6 != "")
			break;
		let out = ctx.popen(sprintf("wget -q -O - -T 3 'http://%s' 2>/dev/null", host));
		out = (type(out) == "string") ? trim(out) : "";
		if (out == "")
			continue;
		let m4 = match(out, /[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+/);
		if (m4 && v4 == "") {
			v4 = m4[0];
		} else if (v6 == "" && index(out, ":") >= 0 && match(out, /^[0-9a-fA-F:]+$/)) {
			v6 = out;
		}
	}
	let list = [];
	if (v4 != "") push(list, v4);
	if (v6 != "") push(list, v6);
	return list;
}

// ---- scan flow --------------------------------------------------------------
function http_post_json(ctx, url, body) {
	// busybox wget: -q quiet, -O - to stdout, -T timeout, repeated --header, --post-data.
	// `body` holds only ints / validated IP strings, so single-quoting is safe.
	let cmd = sprintf("wget -q -O - -T 20 --header='Content-Type: application/json' --header='User-Agent: curl/7.8.0' --header='Accept: application/json' --post-data='%s' '%s' 2>/dev/null",
		body, url);
	let out = ctx.popen(cmd);
	return (type(out) == "string") ? trim(out) : "";
}

// Resolve a scan (middle) server for our region, falling back to the fixed server.
function fetch_scan_server_url(ctx, body) {
	let raw = http_post_json(ctx, SCAN_ENDPOINT, body);
	if (raw != "") {
		let parsed = json(raw);
		let servers = (type(parsed) == "object") ? (parsed.servers ?? (parsed.data ? parsed.data.servers : null)) : null;
		if (type(servers) == "array" && length(servers) > 0) {
			let sel = servers[0];
			if (type(sel) == "object" && sel.middle_server_url)
				return sel.middle_server_url;
		}
	}
	// Using fallback scan server.
	return SCAN_FALLBACK;
}

// Delegate the connect-back probe to the public endpoint and normalise the reply
// into the { ipv4:{verified}, ipv6:{verified}, ... } shape the app expects, or
// { error: "..." } on failure (fed through map_bin_error()).
function run_check(ctx, port, protocol, public_ips) {
	if (length(public_ips) == 0)
		return { error: "no valid ip in response" };

	let ip_json = "";
	for (let ip in public_ips)
		ip_json = ip_json + (ip_json == "" ? "" : ",") + sprintf("\"%s\"", ip);

	let body = sprintf("{\"request_id\":\"%08x%08x\",\"timestamp\":%d,\"port\":%d,\"protocol\":\"%s\",\"public_ip\":[%s]}",
		time(), int(rand()), time(), port, protocol, ip_json);

	let scan_url = fetch_scan_server_url(ctx, body);
	let raw = http_post_json(ctx, scan_url, body);
	if (raw == "")
		return { error: "Scan server request failed: no response" };

	let res = json(raw);
	if (type(res) != "object")
		return { error: "invalid result json" };

	if (res.error)
		return { error: `${res.error}` };
	// Some deployments wrap the useful payload under `data`.
	if (res.data && type(res.data) == "object" && (res.data.ipv4 || res.data.ipv6))
		return res.data;
	return res;
}

export default {
	// Verify that the (currently stopped) VPN server's port is reachable from the
	// internet. param.interface is "ovpnserver" or "wgserver".
	server_connectivity_check: function(param, ctx) {
		if (type(param) != "object")
			return err_pack(E.PARAM_INVALIED);

		if (!acquire_lock(ctx))
			return err_pack(E.OPERATE_TOO_FREQUENT);

		let iface = param.interface;
		if (iface == null) {
			release_lock(ctx);
			return err_pack(E.PARAM_MISSING);
		}

		let vpn_config = get_vpn_config(ctx, iface);
		// DEGRADE: no VPN server configured / no port -> natural VPN error shape.
		if (type(vpn_config) != "object") {
			release_lock(ctx);
			return err_pack(vpn_config ?? E.GET_CONF_FAIL);
		}

		add_firewall_rule(ctx, vpn_config.port, vpn_config.protocol);
		let public_ips = fetch_public_ips(ctx);
		let result = run_check(ctx, vpn_config.port, vpn_config.protocol, public_ips);
		remove_firewall_rule(ctx, vpn_config.port, vpn_config.protocol);

		if (result.error) {
			release_lock(ctx);
			return err_pack(map_bin_error(`${result.error}`));
		}

		let ipv4_verified = result.ipv4 && result.ipv4.verified;
		let ipv6_verified = result.ipv6 && result.ipv6.verified;

		let network_info = get_network_gateway_ip(ctx);
		result.gateway_ip = network_info.gateway_ip;
		result.gateway = result.gateway_ip;

		if (!ipv4_verified && !ipv6_verified) {
			result.dev_info = {
				internal_ip: network_info.internal_ip,
				internal_port: vpn_config.port,
				protocol: [ vpn_config.protocol ]
			};
		}

		release_lock(ctx);
		return result;
	}
};
