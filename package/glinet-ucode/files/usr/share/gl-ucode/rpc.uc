// gl-ucode: JSON-RPC dispatch core (port of oui-rpc.lua + oui/rpc.lua).
// ES module — import { handle } from './rpc.uc'.
// handle(body_string, reqinfo, sink) -> response object.
// reqinfo = { remote_addr, is_local, glinet }   (glinet = bool, "glinet" header present)
'use strict';

import * as util from './lib/util.uc';
import * as session from './lib/session.uc';

// RPC objects — STATIC imports (not loadfile): uhttpd's ucode handler VM aborts on
// loadfile(), and static imports are LSP-resolvable. Add a line here per new object.
import o_system  from './objects/system.uc';
import o_network from './objects/network.uc';
import o_wifi    from './objects/wifi.uc';
import o_clients from './objects/clients.uc';
import o_ui      from './objects/ui.uc';
// Wave 1
import o_diag        from './objects/diag.uc';
import o_igmp        from './objects/igmp.uc';
import o_ipv6        from './objects/ipv6.uc';
import o_led         from './objects/led.uc';
import o_ovpn_client from './objects/ovpn_client.uc';
import o_logread     from './objects/logread.uc';
import o_zerotier    from './objects/zerotier.uc';
import o_sqm         from './objects/sqm.uc';
// Wave 2
import o_adguardhome from './objects/adguardhome.uc';
import o_tor         from './objects/tor.uc';
import o_tailscale   from './objects/tailscale.uc';
import o_ksmbd       from './objects/ksmbd.uc';
import o_luci        from './objects/luci.uc';
import o_localaccess from './objects/local-access.uc';
import o_bwlist      from './objects/black_white_list.uc';
import o_timer       from './objects/timer.uc';
// Wave 3
import o_switchbtn   from './objects/switch-button.uc';
import o_edgerouter  from './objects/edgerouter.uc';
import o_qos         from './objects/qos.uc';
import o_acl         from './objects/acl.uc';
import o_wg_client   from './objects/wg_client.uc';
import o_kmwan       from './objects/kmwan.uc';
import o_tethering   from './objects/tethering.uc';
import o_dns         from './objects/dns.uc';
// Wave 4
import o_lan         from './objects/lan.uc';
import o_firewall    from './objects/firewall.uc';
import o_netmode     from './objects/netmode.uc';
import o_cable       from './objects/cable.uc';
import o_repeater    from './objects/repeater.uc';
import o_vpnclient   from './objects/vpn-client.uc';
import o_upgrade     from './objects/upgrade.uc';
// Wave 5 (DPI trio)
import o_dpi         from './objects/dpi.uc';
import o_flowstats   from './objects/flow_statistics.uc';
import o_parental    from './objects/parental-control.uc';
// Cloud (GoodCloud — RPC surface; needs the gl-cloud daemon + account binding to activate)
import o_cloud       from './objects/cloud.uc';
import o_rtty        from './objects/rtty.uc';
import o_srvconn     from './objects/srv_conn_check.uc';
// dashboard extras + app-facing (hyphenated) names
import o_modem       from './objects/modem.uc';
import o_nasweb      from './objects/nas-web.uc';
import o_macclone    from './objects/macclone.uc';
import o_ddns        from './objects/ddns.uc';

const OBJECTS = {
	system: o_system, network: o_network, wifi: o_wifi, clients: o_clients, ui: o_ui,
	diag: o_diag, igmp: o_igmp, ipv6: o_ipv6, led: o_led, ovpn_client: o_ovpn_client,
	logread: o_logread, zerotier: o_zerotier, sqm: o_sqm,
	adguardhome: o_adguardhome, tor: o_tor, tailscale: o_tailscale, ksmbd: o_ksmbd,
	luci: o_luci, "local-access": o_localaccess, black_white_list: o_bwlist, timer: o_timer,
	"switch-button": o_switchbtn, edgerouter: o_edgerouter, qos: o_qos, acl: o_acl,
	wg_client: o_wg_client, kmwan: o_kmwan, tethering: o_tethering, dns: o_dns,
	lan: o_lan, firewall: o_firewall, netmode: o_netmode, cable: o_cable,
	repeater: o_repeater, "vpn-client": o_vpnclient, upgrade: o_upgrade,
	dpi: o_dpi, flow_statistics: o_flowstats, "parental-control": o_parental,
	cloud: o_cloud, rtty: o_rtty, srv_conn_check: o_srvconn,
	modem: o_modem, "nas-web": o_nasweb, macclone: o_macclone, ddns: o_ddns,
	// the app addresses the VPN client objects by their hyphenated (.so) names
	"wg-client": o_wg_client, "ovpn-client": o_ovpn_client
};

const fs   = require('fs');
const uci  = require('uci');         // stub-seam: tests resolve ./uci.uc via cwd
let   ubus = require('ubus');

const E = util.E;

/**
 * @param {string} s
 */
function valid_name(s) {
	return (type(s) == "string" && match(s, /^[A-Za-z_][A-Za-z0-9_-]+$/));
}

// no-auth allowlist (seed from the stock oui-httpd config; extended from uci if present)
let NO_AUTH = {
	ui: { get_lang:1, load_locales:1, check_initialized:1, get_wifi_config_init:1,
	      init:1, get_timezone_list:1, get_challenge_nonce:1 },
	system: { get_timezone_list:1 }
};

function load_uci_no_auth() {
	let c = uci.cursor();
	let cfg = c?.get_all("oui-httpd");
	if (!cfg) return;
	for (let k in cfg) {
		let s = cfg[k];
		if (s[".type"] != "no-auth-methods") continue;
		let obj = s.object, methods = s.method;
		if (!obj || !methods) continue;
		NO_AUTH[obj] ??= {};
		for (let m in methods) NO_AUTH[obj][m] = 1;
	}
}
load_uci_no_auth();

function is_no_auth(object, method) {
	return (NO_AUTH[object] && NO_AUTH[object][method]) ? true : false;
}

function access(object, method, sess, reqinfo) {
	if (reqinfo.is_local && reqinfo.glinet) return true;
	let aclgroup = sess ? sess.aclgroup : null;
	if (aclgroup == "root") return true;
	if (!aclgroup) return false;
	return false;   // scoped ACL (sqlite) not implemented in v1
}

// Object lookup in the static import table (no loadfile — see the import block).
function load_object(object) {
	return OBJECTS[object] ?? false;
}

function make_ctx(sess, reqinfo) {
	return {
		uci: uci,
		ubus: ubus.connect(),
		fs: fs,
		util: util,
		session: sess,
		reqinfo: reqinfo,
		// convenience: run a command, return trimmed stdout
		popen: function(cmd) {
			let p = fs.popen(cmd, "r"); // files/usr/share/gl-ucode/rpc.uc:80:12: [error] popen() always passes its argument through /bin/sh — there is no array form. If the command string contains any dynamic content, this is a command-injection risk. Sanitize inputs, or replace with system() using an argv array to avoid the shell entirely. (no-unsafe-popen)
			if (!p) return null;
			let out = p.read("all");
			p.close();
			return out ? util.trim(out) : "";
		}
	};
}

/**
 * @param {unknown} id
 * @param {object|array} params
 * @param {object} reqinfo
 */
function do_call(id, params, reqinfo) {
	if (type(params) != "array" || length(params) < 3)
		return util.error_response(id, E.INVALID_PARAMS);
	let sid = params[0], object = params[1], method = params[2], args = params[3] ?? {};
	// The GL app leaves params[0] empty and authenticates via the Admin-Token cookie
	// (set at login). Fall back to that when no sid is given in the params.
	if ((sid == null || sid == "") && reqinfo && reqinfo.token)
		sid = reqinfo.token;
	if (type(sid) != "string" || !valid_name(object) || !valid_name(method))
		return util.error_response(id, E.INVALID_PARAMS);
	if (type(args) != "object")
		return util.error_response(id, E.INVALID_PARAMS);

	let sess = session.get(sid); // (variable) sess: unknown

	if (!is_no_auth(object, method)) {
		if (!access(object, method, sess, reqinfo))
			return util.error_response(id, E.ACCESS);
	}

	let obj = load_object(object);
	if (!obj || type(obj[method]) != "function")
		return util.error_response(id, E.METHOD_NOT_FOUND);

	let res;
	try { res = obj[method](args, make_ctx(sess, reqinfo)); }
	catch (e) {
		warn(`gl-ucode: ${object}.${method} threw: ${e}\n`);
		return util.error_response(id, E.INTERNAL_ERROR, `${e}`);
	}

	if (type(res) == "int")                       // method returned an error code
		return util.error_response(id, res);
	return util.result_response(id, (type(res) == "object" || type(res) == "array") ? res : {});
}

// ---- top-level method handlers ----
function m_challenge(id, p) {
	let r = session.challenge(p ? p.username : null, p ? p.password : null);
	return (r.code != 0) ? util.error_response(id, r.code, r.data)
	                     : util.result_response(id, r.data);
}
function m_login(id, p, reqinfo, sink) {
	let r = session.login(p ? p.username : null, p ? p.hash : null);
	if (r.code != 0) return util.error_response(id, r.code, r.data);
	if (sink) sink.set_cookie = `Admin-Token=${r.data.sid}`;
	return util.result_response(id, r.data);
}
function m_logout(id, p) {
	session.logout(p ? p.sid : null);
	return util.result_response(id);
}
function m_alive(id, p) {
	let r = session.touch(p ? p.sid : null);
	return (r.code != 0) ? util.error_response(id, r.code) : util.result_response(id);
}

// main entry. `sink` (optional) receives out-of-band response hints (e.g. set_cookie).
/**
 * @param {object | string} body
 * @param {object} reqinfo
 * @param {object} sink
 */
export function handle(body, reqinfo, sink) {
	reqinfo ??= { is_local: false, glinet: false };
	let req;
	try { req = json(body); }
	catch (e) { return util.error_response(null, E.PARSE_ERROR); }
	if (type(req) != "object")
		return util.error_response(null, E.PARSE_ERROR);
	if (type(req.method) != "string")
		return util.error_response(req.id, E.INVALID_REQUEST);
	if (req.params != null && type(req.params) != "object" && type(req.params) != "array")
		return util.error_response(req.id, E.INVALID_REQUEST);

	let params = req.params ?? {};
	let resp;
	switch (req.method) {
	case "challenge": resp = m_challenge(req.id, params); break;
	case "login":     resp = m_login(req.id, params, reqinfo, sink); break;
	case "logout":    resp = m_logout(req.id, params); break;
	case "alive":     resp = m_alive(req.id, params); break;
	case "call":      resp = do_call(req.id, params, reqinfo); break;
	default:          resp = util.error_response(req.id, E.METHOD_NOT_FOUND);
	}
	// TEMP tracing (no payloads): object.method -> result code, to see what the app polls.
	if (getenv("GL_RPC_TRACE") != null || fs.access("/tmp/gl-rpc.trace")) {
		let _lf = fs.open("/tmp/gl-rpc.log", "a");
		if (_lf) {
			let tag = (req.method == "call" && type(params) == "array")
				? `call ${params[1]}.${params[2]}` : req.method;
			let ec = (resp && resp.error) ? resp.error.code : "ok";
			_lf.write(sprintf("%d %s -> %s\n", time(), tag, ec));
			_lf.close();
		}
	}
	return resp;
};
