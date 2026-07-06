// gl-ucode object: local-access  (port of decompiled-spec/rpc/local-access.lua)
// Management-access ports (web http/https + ssh) + https-redirect + session
// timeout, plus WAN-access firewall rules and a /proc/net/tcp port-occupancy scan.
//
// RETARGET: the original served the GL web console from nginx and rewrote
// /etc/nginx/conf.d/gl.conf, kept redirect_https/session_timeout in `oui-httpd`,
// and queried LuCI's separate uhttpd via `gl-session`/`oui-httpd` rpc. None of that
// stack exists on mainline. There is exactly ONE web server here: stock uhttpd.
// So http_port/https_port/redirect_https/session_timeout are retargeted onto
// uhttpd `main` (listen_http/listen_https/redirect_https/session_timeout), and the
// luci_* fields (which already targeted uhttpd upstream) are kept as aliases of the
// same single server so the app-facing shape is preserved.
// DROP: nginx conf rewriting, AdGuardHome port probe, tor firewall coupling, and
// the gl_ddns http/https_port mirror -- none of those subsystems exist here.
'use strict';

const fs = require('fs');

// Parse the first port out of a uhttpd listen_http/listen_https option, which is a
// uci list of "addr:port" (e.g. "0.0.0.0:80", "[::]:80"). Returns int or null.
function listen_port(c, opt) {
	let v = c.get("uhttpd", "main", opt);
	if (v == null) return null;
	let s = (type(v) == "array") ? (length(v) ? v[0] : null) : v;
	if (s == null) return null;
	let m = match(s, /:([0-9]+)/);
	return m ? int(m[1]) : null;
}

function get_config(ctx) {
	let c = ctx.uci.cursor();
	let http_port = listen_port(c, "listen_http");
	let https_port = listen_port(c, "listen_https");
	let redirect_https = (c.get("uhttpd", "main", "redirect_https") == "1");
	let session_timeout = int(c.get("uhttpd", "main", "session_timeout") ?? 300);
	let ssh_port = c.get("dropbear", "@dropbear[0]", "Port");
	let ssh_enabled = (c.get("dropbear", "@dropbear[0]", "enable") != "0");
	return {
		redirect_https: redirect_https,
		http_port: http_port,
		https_port: https_port,
		ssh_enabled: ssh_enabled,
		ssh_port: ssh_port != null ? int(ssh_port) : null,
		session_timeout: session_timeout,
		// Single-server retarget: luci_* mirror the same uhttpd instance.
		luci_http_port: http_port,
		luci_https_port: https_port,
		luci_redirect_https: redirect_https
	};
}

function valid_port(p) {
	if (type(p) != "int" && type(p) != "double") return false;
	return p == int(p) && p > 0 && p <= 65535;
}

function hex2int(s) {
	let n = 0;
	s = lc(s);
	for (let i = 0; i < length(s); i++) {
		let d = index("0123456789abcdef", substr(s, i, 1));
		if (d < 0) return -1;
		n = n * 16 + d;
	}
	return n;
}

// True if some socket is LISTENing on `port` (IPv4 or IPv6). Faithful to the
// original /proc/net/tcp scan (state 0A = TCP_LISTEN); AdGuardHome probe dropped.
function port_occupied(port) {
	for (let path in ["/proc/net/tcp", "/proc/net/tcp6"]) {
		let data = fs.readfile(path);
		if (!data) continue;
		for (let l in split(data, "\n")) {
			let m = match(l, /:([0-9A-Fa-f]+) [0-9A-Fa-f]+:[0-9A-Fa-f]+ ([0-9A-Fa-f]+)/);
			if (m && m[2] == "0A" && hex2int(m[1]) == port)
				return true;
		}
	}
	return false;
}

export default {
	get_config: function(args, ctx) {
		return get_config(ctx);
	},

	set_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let old = get_config(ctx);

		let session_timeout = args.session_timeout ?? old.session_timeout ?? 300;
		let redirect_https = args.redirect_https ?? old.redirect_https ?? false;
		let ssh_enabled = args.ssh_enabled ?? old.ssh_enabled ?? false;
		let ssh_port = args.ssh_port ?? old.ssh_port ?? 22;

		// Single web server: a change to either the console or the luci_* alias is
		// a change to uhttpd. Prefer the primary console field, fall back to luci_*.
		let http_port = args.http_port ?? args.luci_http_port ?? old.http_port ?? 80;
		let https_port = args.https_port ?? args.luci_https_port ?? old.https_port ?? 443;
		let web_redirect = args.redirect_https ?? args.luci_redirect_https ?? old.redirect_https ?? false;

		if (type(redirect_https) != "bool")
			return { err_code: -1, err_msg: "invalid redirect_https" };
		if (type(ssh_enabled) != "bool")
			return { err_code: -1, err_msg: "invalid ssh_enabled" };
		if (type(web_redirect) != "bool")
			return { err_code: -1, err_msg: "invalid luci_redirect_https" };
		if (!(valid_port(http_port) && valid_port(https_port) && valid_port(ssh_port)))
			return ctx.util.E.INVALID_PARAMS;

		// Reject moving a service onto a port something else already occupies.
		if (old.http_port != http_port && port_occupied(http_port))
			return { err_code: -1, err_msg: "http port occupied" };
		if (old.https_port != https_port && port_occupied(https_port))
			return { err_code: -2, err_msg: "https port occupied" };
		if (old.ssh_port != ssh_port && port_occupied(ssh_port))
			return { err_code: -3, err_msg: "ssh port occupied" };

		// Web console -> uhttpd main (the real serving stack).
		c.set("uhttpd", "main", "listen_http", [ `0.0.0.0:${http_port}`, `[::]:${http_port}` ]);
		c.set("uhttpd", "main", "listen_https", [ `0.0.0.0:${https_port}`, `[::]:${https_port}` ]);
		c.set("uhttpd", "main", "redirect_https", web_redirect ? "1" : "0");
		c.set("uhttpd", "main", "session_timeout", int(session_timeout));

		// SSH -> dropbear.
		c.set("dropbear", "@dropbear[0]", "Port", int(ssh_port));
		c.set("dropbear", "@dropbear[0]", "enable", ssh_enabled ? "1" : "0");

		// WAN access -> firewall rules (only touch rules that already exist so we
		// never leave a half-formed rule; a missing rule means WAN access is simply
		// not provisioned on this image). tor/gl_ddns/glservice coupling dropped.
		if (ssh_enabled) {
			for (let r in ["ssh_wan", "ssh_wan6"])
				if (c.get("firewall", r) != null)
					c.set("firewall", r, "dest_port", int(ssh_port));
		} else {
			c.delete("firewall", "ssh_wan");
			c.delete("firewall", "ssh_wan6");
		}
		for (let r in ["https_wan", "https_wan6"])
			if (c.get("firewall", r) != null)
				c.set("firewall", r, "dest_port", int(https_port));

		c.commit("uhttpd");
		c.commit("dropbear");
		c.commit("firewall");
		ctx.popen("sync");

		// Restart out-of-band: restarting uhttpd/dropbear tears down live sockets,
		// so background it and let this RPC reply first.
		ctx.popen("(sleep 1; /etc/init.d/dropbear restart; /etc/init.d/uhttpd restart; /etc/init.d/firewall reload) >/dev/null 2>&1 &");
		return {};
	}
};
