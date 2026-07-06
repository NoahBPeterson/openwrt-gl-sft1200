// gl-ucode object: firewall  (port of decompiled-spec/rpc/firewall.lua)
// Port-forwarding / DMZ / custom-rules+ACL / WAN-access. Everything maps onto the
// stock fw4 `firewall` uci (zone/rule/redirect/ipset).
//
// KEY RETARGETS / DEGRADES vs the GL original:
//  - PORT-FORWARD COLLAPSE: GL kept port-forwards (and the DMZ) in a SEPARATE
//    `port_forward` uci config that a helper `/usr/bin/port_forward.lua` translated
//    into `firewall.redirect` sections. That indirection is COLLAPSED here: we read
//    and write `firewall.redirect` sections directly and DROP every
//    `/usr/bin/port_forward.lua` invocation. Field names are unchanged (src, dest,
//    proto, dest_ip, src_dport, dest_port, name, enabled) so the app-facing shape and
//    the GL-/GL-DMZ name prefixes are preserved exactly.
//  - ORDERING: GL's `gl_prio` + oui.utils.adjust_firewall_priority are reworked onto
//    fw4 config-position ordering via uci reorder (ported faithfully below). The
//    `gl_prio` option is still written (1=ACL, 2=normal) so ordering + get_* shape
//    round-trip.
//  - conntrack -D flushes are preserved and run via ctx.popen (backgrounded).
//  - restart_services: GL's nginx / gl-ngx-session / gl_ddns branch is DROPPED (that
//    stack is absent on mainline); dropbear (ssh) + uhttpd (luci web) branches kept.
//  - WAN-access https port comes from stock uhttpd (listen_https) instead of GL's
//    oui-httpd local-access module. Whitelist is stored in `wan-access` uci (GL's own
//    store, created on demand) and enforced through fw4 ipset sections.
//  - gl.validator is replaced with local IPv4/IPv6/CIDR + port validators.
'use strict';

const fs = require('fs');

const CUSTOM_ACL_PREFIX = "GLACL";

// ---- primitive validators (replace gl.validator) --------------------------------

function has_control_chars(value) {
	if (type(value) != "string") return false;
	for (let i = 0; i < length(value); i++) {
		let ch = ord(value, i);
		if (ch < 32 || ch == 127) return true;
	}
	return false;
}

function is_ip4(s) {
	if (type(s) != "string") return false;
	let m = match(s, /^([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})$/);
	if (!m) return false;
	for (let i = 1; i <= 4; i++) if (int(m[i]) > 255) return false;
	return true;
}

// loose IPv6 check (hex groups + at least one colon; dash-free char class)
function is_ip6(s) {
	return type(s) == "string" && index(s, ":") >= 0 && match(s, /^[0-9a-fA-F:.]+$/) != null;
}

function cidr_prefix_ok(s, maxbits) {
	return match(s, /^[0-9]+$/) != null && int(s) >= 0 && int(s) <= maxbits;
}

function is_ip4_or_cidr(s) {
	if (is_ip4(s)) return true;
	if (type(s) != "string") return false;
	let p = split(s, "/");
	return length(p) == 2 && is_ip4(p[0]) && cidr_prefix_ok(p[1], 32);
}

function is_ip6_or_cidr(s) {
	if (is_ip6(s)) return true;
	if (type(s) != "string") return false;
	let p = split(s, "/");
	return length(p) == 2 && is_ip6(p[0]) && cidr_prefix_ok(p[1], 128);
}

function is_ip_or_cidr(s) { return is_ip4_or_cidr(s) || is_ip6_or_cidr(s); }

function is_valid_ip_or_cidr(ip) {
	if (type(ip) != "string" || ip == "" || has_control_chars(ip)) return false;
	return is_ip_or_cidr(ip);
}

function is_valid_clean_ip(ip) {
	if (ip == null || ip == "") return true;
	if (type(ip) != "string" || has_control_chars(ip)) return false;
	return is_ip_or_cidr(ip);
}

function is_valid_clean_port(port) {
	let t = type(port);
	if (t == "int" || t == "double") return port % 1 == 0 && port >= 1 && port <= 65535;
	if (t != "string" || match(port, /^0[0-9]+/)) return false;
	if (!match(port, /^[0-9]+$/)) return false;
	let n = int(port);
	return n >= 1 && n <= 65535;
}

// single port OR "a-b" range (span <= 100). Faithful port of is_valid_port().
function is_valid_port(port) {
	let t = type(port);
	if (port == null || (t != "string" && t != "int" && t != "double")) return false;
	if (t == "string") {
		let m = match(port, /^([0-9]+)-([0-9]+)$/);
		if (m) {
			if (match(m[1], /^0[0-9]+/) || match(m[2], /^0[0-9]+/)) return false;
			let sp = int(m[1]), ep = int(m[2]);
			if (sp < 1 || sp > 65535 || ep < 1 || ep > 65535) return false;
			if (ep - sp + 1 > 100) return false;
			return sp <= ep;
		}
		if (match(port, /^0[0-9]+/)) return false;
	}
	if (t == "int" || t == "double") return port % 1 == 0 && port >= 1 && port <= 65535;
	let sm = match(port, /^([0-9]+)$/);
	if (!sm) return false;
	let n = int(sm[1]);
	return n >= 1 && n <= 65535;
}

function is_valid_proto(proto) {
	return proto == "tcp" || proto == "udp" || proto == "tcp udp" || proto == "icmp" || proto == "all";
}

function is_valid_target(target) {
	return target == "ACCEPT" || target == "REJECT" || target == "DROP";
}

// name may not be empty (empty => "not invalid"), whitespace-only, contain control
// chars, or contain any of  " < > = ; ' # * / ~ $ %  (dash-free char class).
function is_invalid_name(name) {
	if (name == null || name == "") return false;
	if (has_control_chars(name)) return true;
	if (trim(name) == "") return true;
	return match(name, /["<>=;'#*\/~$%]/) != null;
}

function is_valid_rule_params(params, require_target) {
	if (require_target && !is_valid_target(params.target)) return false;
	if (params.target != null && !is_valid_target(params.target)) return false;
	if (params.proto != null && !is_valid_proto(params.proto)) return false;
	if (params.src != null && (type(params.src) != "string" || has_control_chars(params.src))) return false;
	if (params.dest != null && (type(params.dest) != "string" || has_control_chars(params.dest))) return false;
	if (params.src_mac != null && (type(params.src_mac) != "string" || has_control_chars(params.src_mac))) return false;
	if (params.src_ip != null && !is_valid_ip_or_cidr(params.src_ip)) return false;
	if (params.dest_ip != null && !is_valid_ip_or_cidr(params.dest_ip)) return false;
	if (params.src_port != null && params.src_port != "" && !is_valid_port(params.src_port)) return false;
	if (params.dest_port != null && params.dest_port != "" && !is_valid_port(params.dest_port)) return false;
	if (params.enabled != null && type(params.enabled) != "bool") return false;
	return true;
}

function is_valid_dmz_params(params) {
	if (params.enabled != null && type(params.enabled) != "bool") return false;
	if (params.priority != null && type(params.priority) != "bool") return false;
	if (params.enabled && params.dest_ip == null) return false;
	if (params.dest_ip != null && (type(params.dest_ip) != "string" || !is_ip4(params.dest_ip))) return false;
	return true;
}

// port-forward param validator (proto restricted to tcp/udp/tcp udp; ipv4 dest).
function check_params(params) {
	if (params.src != null && (type(params.src) != "string" || has_control_chars(params.src))) return false;
	if (params.dest != null && (type(params.dest) != "string" || has_control_chars(params.dest))) return false;
	let proto = params.proto;
	if (proto != "tcp" && proto != "udp" && proto != "tcp udp") return false;
	if (!is_valid_port(params.src_dport)) return false;
	if (!is_valid_port(params.dest_port)) return false;
	if (params.dest_ip != null && !is_ip4(params.dest_ip)) return false;
	if (type(params.enabled) != "bool") return false;
	return true;
}

// ---- WAN-access validators ------------------------------------------------------

function is_valid_wan_access_ip(ip) {
	return type(ip) == "string" && ip != "" && !has_control_chars(ip) && is_ip_or_cidr(ip);
}
function is_ipv4_wan_access_ip(ip) { return is_ip4_or_cidr(ip); }
function is_ipv6_wan_access_ip(ip) { return is_ip6_or_cidr(ip); }

function is_valid_wan_access_whitelist(whitelist) {
	if (whitelist == null) return true;
	if (type(whitelist) != "array") return false;
	for (let item in whitelist) {
		if (type(item) != "object") return false;
		if (item.name != null && (type(item.name) != "string" || is_invalid_name(item.name))) return false;
		if (!is_valid_wan_access_ip(item.ipaddr)) return false;
	}
	return true;
}

function is_valid_wan_access_params(params) {
	if (type(params) != "object") return false;
	for (let key in ["enable_ping", "enable_https", "enable_ssh", "enable_whitelist"])
		if (params[key] != null && type(params[key]) != "bool") return false;
	return is_valid_wan_access_whitelist(params.whitelist);
}

// ---- conntrack flush command builders (run later via ctx.popen) -----------------

function append_clean_cmd(cmd, clean_cmd) {
	if (!clean_cmd || clean_cmd == "") return cmd;
	if (cmd && cmd != "") return clean_cmd + ";" + cmd;
	return clean_cmd;
}

function generate_clean_cmd(port, ip) {
	let sip = "", dip = "";
	if (port == null || port == "" || !is_valid_clean_port(port)) return "";
	port = `${port}`;
	if (ip != null && ip != "") {
		if (!is_valid_clean_ip(ip)) return "";
		sip = "-s " + ip;
		dip = "-d " + ip;
	}
	let cmd = sprintf("conntrack -D %s -p tcp --dport %s 2>/dev/null", sip, port);
	cmd += sprintf(";conntrack -D %s -p tcp --sport %s 2>/dev/null", dip, port);
	cmd += sprintf(";conntrack -D %s -p udp --dport %s 2>/dev/null", sip, port);
	cmd += sprintf(";conntrack -D %s -p udp --sport %s 2>/dev/null", dip, port);
	return cmd;
}

function generate_dmz_clean_cmd(ip) {
	if (ip == null || ip == "" || !is_valid_clean_ip(ip)) return "";
	return "conntrack -D -s " + ip + " 2>/dev/null;conntrack -D -d " + ip + "  2>/dev/null";
}

function generate_tcp_clean_cmd(port) {
	if (!is_valid_clean_port(port)) return "";
	port = `${port}`;
	return sprintf("conntrack -D -p tcp --sport %s 2>/dev/null; conntrack -D -p tcp --dport %s 2>/dev/null", port, port);
}

// ---- ordering: fw4 config-position rework of gl_prio ----------------------------

// Faithful port of oui.utils.adjust_firewall_priority: reorder `sid` among the
// firewall `rule` sections so lower gl_prio (1=ACL, 2=normal) sits earlier. A rule
// is dropped in just after the last rule of the SAME prio, else just before the
// first rule of LOWER prio (higher number). Config position IS the fw4 eval order.
function adjust_firewall_priority(ctx, sid) {
	let c = ctx.uci.cursor();
	const special_rules = { sambasharewan: 2, sambasharelan: 2, glnas_ser: 2, webdav_wan: 2 };
	let my_prio = null;
	c.foreach("firewall", "rule", function(s) {
		if (s[".name"] == sid) my_prio = (s.gl_prio != null) ? int(s.gl_prio) : null;
	});
	if (my_prio == null) return;
	let last_same_prio_pos = null;
	let first_lower_prio_pos = null;
	c.foreach("firewall", "rule", function(s) {
		if (s[".name"] == sid) return;
		let rule_prio = (s.gl_prio != null) ? int(s.gl_prio) : special_rules[s[".name"]];
		if (rule_prio == null) return;
		if (rule_prio == my_prio) last_same_prio_pos = s[".index"];
		else if (rule_prio > my_prio && first_lower_prio_pos == null) first_lower_prio_pos = s[".index"];
	});
	if (last_same_prio_pos != null) c.reorder("firewall", sid, last_same_prio_pos + 1);
	else if (first_lower_prio_pos != null) c.reorder("firewall", sid, first_lower_prio_pos);
	c.commit("firewall");
}

// Keep the GL-DMZ redirect at the top (high prio) or bottom (GL-DMZ-low) of the
// redirect list. Port of adjust_dmz_prio, retargeted to the firewall config.
function adjust_dmz_prio(c) {
	let sid = null, high = null;
	c.foreach("firewall", "redirect", function(s) {
		if (!s.name || substr(s.name, 0, 6) != "GL-DMZ") return;
		sid = s[".name"];
		high = (s.name != "GL-DMZ-low");
	});
	if (sid != null && high == false) {
		let idx = null;
		c.foreach("firewall", "redirect", function(s) { idx = s[".index"]; });
		if (idx != null) c.reorder("firewall", sid, idx + 1);
	} else if (sid != null) {
		c.reorder("firewall", sid, 0);
	}
}

// ---- section writers/readers (name is always GL- prefixed) ----------------------

// write every param as a uci option (bool -> "1"/"0"); skip meta keys id/apply/all.
function set_opt_fields(c, sid, params) {
	params.name = "GL-" + (params.name ?? "");
	for (let k in params) {
		if (k == "id" || k == "apply" || k == "all") continue;
		let v = params[k];
		if (type(v) == "bool") v = v ? "1" : "0";
		c.set("firewall", sid, k, v);
	}
}

// GL "rule" writer: tags gl_prio=2 then writes the fields.
function set_opt(c, sid, params) {
	c.set("firewall", sid, "gl_prio", "2");
	set_opt_fields(c, sid, params);
}

// redirect (port-forward / DMZ) writer -- no gl_prio.
function set_opt_port(c, sid, params) {
	set_opt_fields(c, sid, params);
}

// Read GL- prefixed sections of `typ`, strip the prefix, normalise enabled/id.
// `filter(obj)` may return false to skip (called on the prefix-stripped object).
function get_gl_sections(c, typ, filter) {
	let res = [];
	c.foreach("firewall", typ, function(s) {
		let name = s.name;
		if (!name) return;
		if (substr(name, 0, 3) != "GL-") return;
		let obj = {};
		for (let k in s) {
			if (k == ".name" || k == ".anonymous" || k == ".index" || k == ".type" || k == "name")
				continue;
			obj[k] = s[k];
		}
		let stripped = substr(name, 3);
		if (stripped != "") obj.name = stripped;   // empty name => absent (matches nil)
		if (filter != null && filter(obj) === false) return;
		obj.enabled = (s.enabled != "0");
		obj.id = s[".name"];
		push(res, obj);
	});
	return res;
}

// ---- management-port helpers (mainline: dropbear + uhttpd) ----------------------

function uhttpd_port(c, opt, dflt) {
	let v = c.get("uhttpd", "main", opt);
	let s = (type(v) == "array") ? (length(v) ? v[0] : null) : v;
	if (type(s) != "string") return dflt;
	let m = match(s, /:([0-9]+)/);
	return m ? m[1] : dflt;
}

// Restart a management service if a firewall rule collided with its port. GL's
// nginx/gl_ddns branch is dropped; dropbear + uhttpd(luci) branches kept.
function restart_services(ctx, c, port, enable, old_port) {
	let ssh_port = c.get("dropbear", "@dropbear[0]", "Port");
	if (!(port != ssh_port || enable) || (old_port != port && old_port == ssh_port)) {
		ctx.popen("(sleep 1; /etc/init.d/dropbear shutdown; /etc/init.d/dropbear restart) >/dev/null 2>&1 &");
		return;
	}
	// DROP: gl_ddns http/https + nginx / gl-ngx-session branch (absent on mainline).
	let lh = uhttpd_port(c, "listen_http", "8080");
	let ls = uhttpd_port(c, "listen_https", "8443");
	if ((!enable && (port == lh || port == ls)) || (old_port != port && (old_port == lh || old_port == ls))) {
		ctx.popen("(sleep 1; rm -rf /tmp/luci*; /etc/init.d/uhttpd restart 2>/dev/null; /etc/init.d/rpcd restart 2>/dev/null) >/dev/null 2>&1 &");
		return;
	}
}

// Toggle the FTP conntrack helper based on whether an FTP port-forward or the DMZ
// is enabled (mirrors set_nf_conntrack_helper, retargeted to firewall.redirect).
function set_nf_conntrack_helper(ctx) {
	const proc = "/proc/sys/net/netfilter/nf_conntrack_helper";
	if (!fs.access(proc)) return;
	let c = ctx.uci.cursor();
	let ftp = false;
	c.foreach("firewall", "redirect", function(s) {
		if (s.dest_port == "21" && s.enabled == "1") ftp = true;
		if (s.name && substr(s.name, 0, 6) == "GL-DMZ" && s.enabled == "1") ftp = true;
	});
	if (ftp) {
		ctx.popen(`echo 1 > ${proc} 2>/dev/null`);
	} else {
		let sip_alg = c.get("sip_alg", "global", "enable");   // GL config, degrade -> null
		if (sip_alg != "1") ctx.popen(`echo 0 > ${proc} 2>/dev/null`);
	}
}

// Signal fw4 to reload (ubus service config.change) + refresh conntrack helper +
// run any queued conntrack flush. Port of firewall_apply.
function firewall_apply(ctx, apply, cmd) {
	if (apply !== false)
		ctx.ubus.call("service", "event", { type: "config.change", data: { package: "firewall" } });
	set_nf_conntrack_helper(ctx);
	if (cmd && cmd != "")
		ctx.popen(`(sleep 1; ${cmd}) >/dev/null 2>&1 &`);
}

// ---- ACL (custom firewall rule) helpers -----------------------------------------

function validate_port_spec(port_str, allow_ranges) {
	if (type(port_str) != "string") return false;
	if (port_str == "") return true;
	let s = trim(port_str);
	if (s == "") return false;
	if (!allow_ranges) {
		if (index(s, ",") >= 0 || index(s, "-") >= 0) return false;
		let single = match(s, /^([0-9]+)$/);
		if (single) {
			let p = int(single[1]);
			if (p >= 1 && p <= 65535) return true;
		}
		return false;
	}
	for (let seg in split(s, ",")) {
		seg = trim(seg);
		let single = match(seg, /^([0-9]+)$/);
		if (single) {
			let p = int(single[1]);
			if (p < 1 || p > 65535) return false;
		} else {
			let r = match(seg, /^([0-9]+)-([0-9]+)$/);
			if (r) {
				let a = int(r[1]), b = int(r[2]);
				if (a < 1 || a > 65535) return false;
				if (b < 1 || b > 65535) return false;
				if (a > b) return false;
			} else {
				return false;
			}
		}
	}
	return true;
}

// non-empty ip list (array with a non-empty first element, or a non-empty string)
function nonempty_iplist(v) {
	if (v == null) return false;
	if (type(v) == "array") return length(v) > 0 && v[0] != "";
	return v != "";
}

// Apply an ACL rule's fields. proto is a uci list (array). Faithful port including
// the (deliberately asymmetric) src/dest "Device"/"" mapping.
function apply_rule_fields(c, rule_name, param) {
	c.set("firewall", rule_name, "gl_prio", "1");
	if (param.target) c.set("firewall", rule_name, "target", param.target);
	if (param.src != null) {
		if (param.src == "") c.set("firewall", rule_name, "src", "*");
		else if (param.src == "Device") c.delete("firewall", rule_name, "src");
		else c.set("firewall", rule_name, "src", param.src);
	}
	if (param.dest != null) {
		if (param.dest == "") c.delete("firewall", rule_name, "dest");
		else if (param.dest == "Device") c.set("firewall", rule_name, "dest", "*");
		else c.set("firewall", rule_name, "dest", param.dest);
	}
	if (param.dest_port != null && param.dest_port != "")
		c.set("firewall", rule_name, "dest_port", param.dest_port);
	else
		c.delete("firewall", rule_name, "dest_port");
	if (param.src_port != null && param.src_port != "")
		c.set("firewall", rule_name, "src_port", param.src_port);
	else
		c.delete("firewall", rule_name, "src_port");
	if (param.proto != null) {
		c.set("firewall", rule_name, "proto", param.proto);
		if (param.proto[0] == "icmp") {
			c.delete("firewall", rule_name, "dest_port");
			c.delete("firewall", rule_name, "src_port");
		}
	}
	if (nonempty_iplist(param.src_ip)) c.set("firewall", rule_name, "src_ip", param.src_ip);
	else c.delete("firewall", rule_name, "src_ip");
	if (nonempty_iplist(param.dest_ip)) c.set("firewall", rule_name, "dest_ip", param.dest_ip);
	else c.delete("firewall", rule_name, "dest_ip");
	if (param.family == "ipv4" || param.family == "ipv6" || param.family == "ipv4 ipv6") {
		if (param.family == "ipv4 ipv6") c.delete("firewall", rule_name, "family");
		else c.set("firewall", rule_name, "family", param.family);
	}
	if (param.enabled != null) c.set("firewall", rule_name, "enabled", param.enabled ? "1" : "0");
	if (param.name) c.set("firewall", rule_name, "name", CUSTOM_ACL_PREFIX + "_" + param.name);
	return true;
}

// zone list for ACL validation (dash-free; excludes ipv6 + overlay zones).
function acl_zone_list(c) {
	let internals = [], externals = [];
	c.foreach("firewall", "zone", function(s) {
		if (s.family == "ipv6" || s.name == "zerotier" || s.name == "tailscale0" ||
		    s.name == "wpan" || s.name == "mptun" || s.name == "wan_all") return;
		push(internals, s.name);
		push(externals, s.name);
	});
	return { internals: internals, externals: externals };
}

// Returns [ok, err]. Faithful port of validate_firewall_rule.
function validate_firewall_rule(c, param) {
	if (!param.target || (param.target != "ACCEPT" && param.target != "DROP" && param.target != "REJECT"))
		return [false, "target error"];
	if (param.enabled && type(param.enabled) != "bool")
		return [false, "enabled error"];
	if (param.src_ip != null && param.src_ip[0] != "") {
		let ip_list = (type(param.src_ip) == "array") ? param.src_ip : [param.src_ip];
		for (let ip in ip_list)
			if (ip != null && ip != "" && !is_ip_or_cidr(ip))
				return [false, "src_ip error: " + ip];
	}
	if (param.dest_ip != null && param.dest_ip[0] != "") {
		let ip_list = (type(param.dest_ip) == "array") ? param.dest_ip : [param.dest_ip];
		for (let ip in ip_list)
			if (ip != null && ip != "" && !is_ip_or_cidr(ip))
				return [false, "dest_ip error: " + ip];
	}
	if (param.dest_port != null && param.dest_port != "" && !validate_port_spec(param.dest_port, true))
		return [false, "dest_port error"];
	if (param.src_port != null && param.src_port != "" && !validate_port_spec(param.src_port, true))
		return [false, "src_port error"];
	if (param.family != null && param.family != "" && param.family != "ipv4" &&
	    param.family != "ipv6" && param.family != "ipv4 ipv6")
		return [false, "family error"];
	let zone_list = acl_zone_list(c);
	if (param.dest != null && param.dest != "" && param.dest != "Device") {
		let found = false;
		for (let z in zone_list.internals) if (z == param.dest) found = true;
		if (!found) return [false, "dest invalid"];
	}
	if (param.src != null && param.src != "" && param.src != "Device") {
		let found = false;
		for (let z in zone_list.internals) if (z == param.src) found = true;
		if (!found) return [false, "src invalid"];
	}
	if (!param.name || length(param.name) == 0) return [false, "name is required"];
	if (param.enabled == null) return [false, "enabled is required"];
	if (length(param.name) > 32) return [false, "name length must be <= 32"];
	return [true, null];
}

// Normalise the app's proto string into a uci list, in place.
function normalize_acl_proto(param) {
	if (param.proto == "tcp udp") param.proto = ["tcp", "udp"];
	else if (param.proto == "tcp") param.proto = ["tcp"];
	else if (param.proto == "udp") param.proto = ["udp"];
	else if (param.proto == "icmp") param.proto = ["icmp"];
	else if (param.proto == "") param.proto = ["all"];
}

export default {
	get_zone_list: function(args, ctx) {
		let c = ctx.uci.cursor();
		let internals = [], externals = [];
		c.foreach("firewall", "zone", function(s) {
			if (s.family == "ipv6" || s.name == "zerotier" || s.name == "tailscale0" ||
			    s.name == "wpan" || s.name == "mptun" || s.name == "wan_all" || s.name == "iot") return;
			push(internals, s.name);
			push(externals, s.name);
		});
		return { internals: internals, externals: externals };
	},

	get_rule_list: function(args, ctx) {
		let c = ctx.uci.cursor();
		return { res: get_gl_sections(c, "rule", null) };
	},

	add_rule: function(args, ctx) {
		let params = args;
		let c = ctx.uci.cursor();
		if (is_invalid_name(params.name))
			return { err_code: -1, err_msg: "invalid name" };
		if (!is_valid_rule_params(params, true))
			return { err_code: 200037001, err_msg: "invalid parameter" };
		let apply = params.apply;
		let sid = c.add("firewall", "rule");
		set_opt(c, sid, params);
		c.commit("firewall");
		adjust_firewall_priority(ctx, sid);
		ctx.popen("sync");
		firewall_apply(ctx, apply);
		return { id: sid };
	},

	set_rule: function(args, ctx) {
		let params = args;
		let cmd;
		let sid = params.id;
		if (type(sid) != "string")
			return ctx.util.E.INVALID_PARAMS;
		if (is_invalid_name(params.name))
			return { err_code: -1, err_msg: "invalid name" };
		if (!is_valid_rule_params(params, false))
			return { err_code: 200037001, err_msg: "invalid parameter" };
		let c = ctx.uci.cursor();
		let old_enabled = c.get("firewall", sid, "enabled");
		let old_dest_port = c.get("firewall", sid, "dest_port");
		let old_proto = c.get("firewall", sid, "proto");
		if (old_enabled == "1" && (`${params.enabled}` == "false" ||
		    `${params.dest_port}` != old_dest_port ||
		    (params.proto != old_proto && params.proto != "tcp udp")))
			cmd = generate_clean_cmd(old_dest_port);
		set_opt(c, sid, params);
		c.commit("firewall");
		ctx.popen("sync");
		restart_services(ctx, c, `${params.dest_port}`, params.enabled, old_dest_port);
		firewall_apply(ctx, null, cmd);
		return {};
	},

	remove_rule: function(args, ctx) {
		let params = args;
		let cmd;
		let c = ctx.uci.cursor();
		if (params.all) {
			let victims = [];
			c.foreach("firewall", "rule", function(s) {
				if (!s.name || substr(s.name, 0, 3) != "GL-") return;
				let old_dest_port = s.dest_port;
				cmd = cmd ? append_clean_cmd(cmd, generate_clean_cmd(old_dest_port))
				          : generate_clean_cmd(old_dest_port);
				push(victims, s[".name"]);
				restart_services(ctx, c, old_dest_port, false);
			});
			for (let id in victims) c.delete("firewall", id);
		} else {
			if (type(params.id) != "string" || !c.get("firewall", params.id))
				return ctx.util.E.INVALID_PARAMS;
			let old_dest_port = c.get("firewall", params.id, "dest_port");
			cmd = generate_clean_cmd(old_dest_port);
			c.delete("firewall", params.id);
			restart_services(ctx, c, old_dest_port, false);
		}
		c.commit("firewall");
		ctx.popen("sync");
		firewall_apply(ctx, params.apply, cmd);
		return {};
	},

	get_dmz: function(args, ctx) {
		let c = ctx.uci.cursor();
		let res = { enabled: false };
		c.foreach("firewall", "redirect", function(s) {
			if (!s.name || substr(s.name, 0, 6) != "GL-DMZ") return;
			res.enabled = (s.enabled != "0");
			res.dest_ip = s.dest_ip;
			res.priority = (s.name != "GL-DMZ-low");
		});
		return res;
	},

	set_dmz: function(args, ctx) {
		let params = args;
		let cmd;
		let c = ctx.uci.cursor();
		let old_name = null;
		let old = { enabled: false };
		let dest_ip = params.dest_ip;
		if (!is_valid_dmz_params(params))
			return ctx.util.E.INVALID_PARAMS;
		let victims = [];
		c.foreach("firewall", "redirect", function(s) {
			if (!s.name || substr(s.name, 0, 6) != "GL-DMZ") return;
			old_name = s.name;
			old.enabled = (s.enabled != "0");
			old.dest_ip = s.dest_ip;
			push(victims, s[".name"]);
		});
		for (let id in victims) c.delete("firewall", id);
		if (old.enabled && (!params.enabled || old.dest_ip != params.dest_ip))
			cmd = generate_dmz_clean_cmd(old.dest_ip);
		let sid = c.add("firewall", "redirect");
		let name = "DMZ";
		if (!params.priority) name = "DMZ-low";
		if (params.priority == null && old_name != null) name = substr(old_name, 3);
		set_opt_port(c, sid, {
			name: name, src: "wan", dest: "lan", proto: "all",
			dest_ip: dest_ip, enabled: params.enabled
		});
		// (GL's separate high-prio reorder among port_forward "rule" sections was a
		// no-op on the old device -- adjust_dmz_prio handles top/bottom placement.)
		adjust_dmz_prio(c);
		c.commit("firewall");
		ctx.popen("sync");
		firewall_apply(ctx, params.apply, cmd);
		return {};
	},

	get_port_forward_list: function(args, ctx) {
		let c = ctx.uci.cursor();
		let res = get_gl_sections(c, "redirect", function(s) {
			if (s.name && substr(s.name, 0, 3) == "DMZ") return false;
		});
		return { res: res };
	},

	add_port_forward: function(args, ctx) {
		let params = args;
		let c = ctx.uci.cursor();
		if (is_invalid_name(params.name))
			return { err_code: -1, err_msg: "invalid name" };
		if (!check_params(params))
			return { err_code: 200037001, err_msg: "invalid parameter" };
		let apply = params.apply;
		let sid = c.add("firewall", "redirect");
		set_opt_port(c, sid, params);
		adjust_dmz_prio(c);
		c.commit("firewall");
		ctx.popen("sync");
		firewall_apply(ctx, apply);
		return { id: sid };
	},

	set_port_forward: function(args, ctx) {
		let params = args;
		let cmd;
		let sid = params.id;
		if (is_invalid_name(params.name))
			return { err_code: -1, err_msg: "invalid name" };
		if (!check_params(params))
			return { err_code: 200037001, err_msg: "invalid parameter" };
		if (type(sid) != "string")
			return ctx.util.E.INVALID_PARAMS;
		let c = ctx.uci.cursor();
		let old_enabled = c.get("firewall", sid, "enabled");
		let old_proto = c.get("firewall", sid, "proto");
		let old_ip = c.get("firewall", sid, "dest_ip");
		let old_port = c.get("firewall", sid, "src_dport");
		if (old_enabled && (!params.enabled || old_ip != params.dest_ip ||
		    old_port != params.dest_port || old_proto != params.proto))
			cmd = generate_clean_cmd(old_port);
		set_opt_port(c, sid, params);
		c.commit("firewall");
		ctx.popen("sync");
		firewall_apply(ctx, null, cmd);
		return {};
	},

	remove_port_forward: function(args, ctx) {
		let params = args;
		let cmd;
		let c = ctx.uci.cursor();
		if (params.all) {
			let victims = [];
			c.foreach("firewall", "redirect", function(s) {
				if (!s.name || substr(s.name, 0, 6) == "GL-DMZ") return;
				if (s.enabled != "0")
					cmd = cmd ? append_clean_cmd(cmd, generate_clean_cmd(s.dest_port))
					          : generate_clean_cmd(s.dest_port);
				push(victims, s[".name"]);
			});
			for (let id in victims) c.delete("firewall", id);
		} else {
			let old_enabled = c.get("firewall", params.id, "enabled");
			let old_ip = c.get("firewall", params.id, "dest_ip");
			let old_port = c.get("firewall", params.id, "dest_port");
			if (old_enabled != "0")
				cmd = generate_clean_cmd(old_port, old_ip);
			c.delete("firewall", params.id);
		}
		c.commit("firewall");
		ctx.popen("sync");
		firewall_apply(ctx, params.apply, cmd);
		return {};
	},

	order_port_forward: function(args, ctx) {
		let params = args;
		let c = ctx.uci.cursor();
		if (type(params.id_list) != "array") return {};
		let rule_index = null;
		c.foreach("firewall", "redirect", function(s) { rule_index = s[".index"]; });
		let idx = 1;
		for (let id in params.id_list) {
			c.foreach("firewall", "redirect", function(s) {
				let sid = s[".name"];
				if (type(id) == "string" && sid == id && rule_index != null) {
					c.reorder("firewall", sid, rule_index + idx);
					idx++;
				}
			});
		}
		adjust_dmz_prio(c);
		c.commit("firewall");
		ctx.popen("sync");
		firewall_apply(ctx, params.apply);
		return {};
	},

	get_wan_access: function(args, ctx) {
		let c = ctx.uci.cursor();
		let ping = c.get("firewall", "ping_wan") != null;
		let https = c.get("firewall", "https_wan") != null;
		let ssh = c.get("firewall", "ssh_wan") != null;
		let enable_whitelist = (c.get("wan-access", "@main[0]", "whitelist") == "1");
		let whitelist = [];
		c.foreach("wan-access", "whitelist", function(s) {
			push(whitelist, { name: s.name, ipaddr: s.ipaddr });
		});
		return {
			enable_ping: ping,
			enable_https: https,
			enable_ssh: ssh,
			enable_whitelist: enable_whitelist,
			whitelist: whitelist
		};
	},

	set_wan_access: function(args, ctx) {
		let params = args;
		let cmd;
		if (!is_valid_wan_access_params(params))
			return ctx.util.E.INVALID_PARAMS;
		let enable_ping = params.enable_ping;
		let enable_https = params.enable_https;
		let enable_ssh = params.enable_ssh;
		let enable_whitelist = params.enable_whitelist;
		let whitelist = params.whitelist ?? [];
		let c = ctx.uci.cursor();
		let old_port = c.get("firewall", "ssh_wan", "dest_port");

		for (let name in ["ping_wan", "ping_wan6", "https_wan", "https_wan6",
		                   "ssh_wan", "ssh_wan6", "whitelist_wan_ipset", "whitelist_wan_ipset6"])
			c.delete("firewall", name);
		let wl_victims = [];
		c.foreach("wan-access", "whitelist", function(s) { push(wl_victims, s[".name"]); });
		for (let id in wl_victims) c.delete("wan-access", id);

		if (enable_ping) {
			for (let name in ["ping_wan", "ping_wan6"]) {
				c.set("firewall", name, "rule");
				c.set("firewall", name, "src", "wan");
				c.set("firewall", name, "proto", "icmp");
				c.set("firewall", name, "icmp_type", "echo-request");
				c.set("firewall", name, "target", "ACCEPT");
				c.set("firewall", name, "gl_prio", "2");
				if (name == "ping_wan6") c.set("firewall", name, "family", "ipv6");
				c.commit("firewall");
				adjust_firewall_priority(ctx, name);
			}
		}
		if (enable_https) {
			// DEGRADE: GL read cfg.https_port from oui-httpd local-access; here it is
			// the stock uhttpd listen_https port.
			let https_port = uhttpd_port(c, "listen_https", "443");
			for (let name in ["https_wan", "https_wan6"]) {
				c.set("firewall", name, "rule");
				c.set("firewall", name, "src", "wan");
				c.set("firewall", name, "proto", "tcp");
				c.set("firewall", name, "dest_port", https_port);
				c.set("firewall", name, "target", "ACCEPT");
				c.set("firewall", name, "gl_prio", "2");
				if (name == "https_wan6") c.set("firewall", name, "family", "ipv6");
				c.commit("firewall");
				adjust_firewall_priority(ctx, name);
			}
		}
		if (enable_ssh) {
			let port = c.get("dropbear", "@dropbear[0]", "Port");
			if (old_port != null && port != old_port)
				cmd = generate_tcp_clean_cmd(old_port);
			for (let name in ["ssh_wan", "ssh_wan6"]) {
				c.set("firewall", name, "rule");
				c.set("firewall", name, "src", "wan");
				c.set("firewall", name, "proto", "tcp");
				c.set("firewall", name, "dest_port", port);
				c.set("firewall", name, "target", "ACCEPT");
				c.set("firewall", name, "gl_prio", "2");
				if (name == "ssh_wan6") c.set("firewall", name, "family", "ipv6");
				c.commit("firewall");
				adjust_firewall_priority(ctx, name);
			}
		} else if (old_port != null) {
			cmd = generate_tcp_clean_cmd(old_port);
		}

		c.set("wan-access", "@main[0]", "whitelist", enable_whitelist ? 1 : 0);
		if (enable_whitelist) {
			let ver = fs.readfile("/proc/version") ?? "";
			let vm = match(ver, /Linux version ([0-9]+\.[0-9]+)/);
			let kernel_version = vm ? (+vm[1]) : 0;
			for (let name in ["whitelist_wan_ipset", "whitelist_wan_ipset6"]) {
				c.set("firewall", name, "ipset");
				if (name == "whitelist_wan_ipset6") {
					c.set("firewall", name, "family", "ipv6");
					c.set("firewall", name, "name", "whitelist_wan6");
				} else {
					c.set("firewall", name, "name", "whitelist_wan");
				}
				if (kernel_version && kernel_version <= 5.4)
					c.set("firewall", name, "match", "src_ip");
				else
					c.set("firewall", name, "match", "src_net");
				if (!fs.access("/sbin/fw4"))
					c.set("firewall", name, "storage", "hash");
			}
		}
		for (let name in ["ping", "https", "ssh"]) {
			if (enable_whitelist) {
				c.set("firewall", name + "_wan", "ipset", "whitelist_wan");
				c.set("firewall", name + "_wan6", "ipset", "whitelist_wan6");
			} else {
				c.delete("firewall", name + "_wan", "ipset");
				c.delete("firewall", name + "_wan6", "ipset");
			}
		}

		let whitelist_ips = [], whitelist_ips_6 = [];
		for (let ip in whitelist) {
			if (is_ipv4_wan_access_ip(ip.ipaddr)) push(whitelist_ips, ip.ipaddr);
			else if (is_ipv6_wan_access_ip(ip.ipaddr)) push(whitelist_ips_6, ip.ipaddr);
			let sid = c.add("wan-access", "whitelist");
			c.set("wan-access", sid, "name", ip.name ?? "");
			c.set("wan-access", sid, "ipaddr", ip.ipaddr);
		}
		if (length(whitelist_ips) > 0)
			c.set("firewall", "whitelist_wan_ipset", "entry", whitelist_ips);
		if (length(whitelist_ips_6) > 0)
			c.set("firewall", "whitelist_wan_ipset6", "entry", whitelist_ips_6);

		c.commit("firewall");
		c.commit("wan-access");
		ctx.popen("sync");

		let flush = cmd ? cmd : "";
		ctx.popen(`(sleep 1; /etc/init.d/firewall restart 2>/dev/null; sleep 1; conntrack -D -p icmp 2>/dev/null; ${flush}) >/dev/null 2>&1 &`);
		if (enable_whitelist && c.get("firewall", "ssh_wan") != null)
			ctx.popen("(sleep 1; /etc/init.d/dropbear shutdown; /etc/init.d/dropbear restart) >/dev/null 2>&1 &");
		return {};
	},

	get_acl_rule_list: function(args, ctx) {
		let c = ctx.uci.cursor();
		let rules = [];
		c.foreach("firewall", "rule", function(s) {
			if (!s.name || substr(s.name, 0, 5) != CUSTOM_ACL_PREFIX) return;
			let enabled = (s.enabled == null) ? true : (s.enabled == "1");
			let proto = (type(s.proto) == "array") ? s.proto : (s.proto != null ? [s.proto] : []);
			let protoStr;
			if (length(proto) == 1) {
				if (proto[0] == "tcp") protoStr = "tcp";
				else if (proto[0] == "udp") protoStr = "udp";
				else if (proto[0] == "icmp") protoStr = "icmp";
				else if (proto[0] == "all") protoStr = "";
				else protoStr = proto[0];
			} else if (length(proto) == 2) {
				let p1 = proto[0], p2 = proto[1];
				if ((p1 == "tcp" && p2 == "udp") || (p1 == "udp" && p2 == "tcp")) protoStr = "tcp udp";
			} else {
				protoStr = "";
			}
			let src = s.src, dest = s.dest;
			if (src == "*") src = "";
			if (dest == "*") dest = "";
			push(rules, {
				id: s[".name"],
				enabled: enabled,
				proto: protoStr,
				dest: dest ?? "Device",
				dest_port: s.dest_port ?? "",
				dest_ip: s.dest_ip ?? [""],
				src: src ?? "Device",
				src_port: s.src_port ?? "",
				src_ip: s.src_ip ?? [""],
				target: s.target,
				family: s.family ?? "ipv4 ipv6",
				name: substr(s.name, 6)
			});
		});
		return rules;
	},

	add_acl_rule: function(args, ctx) {
		let param = args;
		let c = ctx.uci.cursor();
		let vr = validate_firewall_rule(c, param);
		if (!vr[0])
			return { err_code: ctx.util.E.INVALID_PARAMS, err_msg: vr[1] };
		normalize_acl_proto(param);
		param.enabled = (param.enabled !== false);
		let sid = c.add("firewall", "rule");
		apply_rule_fields(c, sid, param);
		c.commit("firewall");
		adjust_firewall_priority(ctx, sid);
		ctx.popen("/etc/init.d/firewall reload 2>/dev/null");
		if (param.enabled !== false && (param.target == "DROP" || param.target == "REJECT"))
			ctx.popen("conntrack -D 2>/dev/null");
		return {};
	},

	edit_acl_rule: function(args, ctx) {
		let param = args;
		let c = ctx.uci.cursor();
		let vr = validate_firewall_rule(c, param);
		normalize_acl_proto(param);
		if (!vr[0])
			return { err_code: ctx.util.E.INVALID_PARAMS, err_msg: vr[1] };
		let cmd;
		let sid = param.id;
		let old_enabled = c.get("firewall", sid, "enabled");
		let old_proto = c.get("firewall", sid, "proto");
		let old_ip = c.get("firewall", sid, "dest_ip");
		let old_port = c.get("firewall", sid, "src_dport");
		apply_rule_fields(c, sid, param);
		if (old_enabled && (!param.enabled || old_ip != param.dest_ip ||
		    old_port != param.dest_port || old_proto != param.proto))
			cmd = generate_clean_cmd(old_port);
		c.commit("firewall");
		firewall_apply(ctx, null, cmd);
		ctx.popen("/etc/init.d/firewall reload 2>/dev/null");
		if (param.enabled !== false && (param.target == "DROP" || param.target == "REJECT"))
			ctx.popen("conntrack -D 2>/dev/null");
		return {};
	},

	delete_acl_rule: function(args, ctx) {
		let params = args;
		let cmd;
		let c = ctx.uci.cursor();
		if (params.all) {
			let victims = [];
			c.foreach("firewall", "rule", function(s) {
				if (!s.name || substr(s.name, 0, 5) != CUSTOM_ACL_PREFIX) return;
				if (s.enabled != "0")
					cmd = cmd ? append_clean_cmd(cmd, generate_clean_cmd(s.dest_port))
					          : generate_clean_cmd(s.dest_port);
				push(victims, s[".name"]);
			});
			for (let id in victims) c.delete("firewall", id);
		} else {
			let rule_name = c.get("firewall", params.id, "name");
			if (rule_name && substr(rule_name, 0, 5) == CUSTOM_ACL_PREFIX) {
				let old_enabled = c.get("firewall", params.id, "enabled");
				let old_ip = c.get("firewall", params.id, "dest_ip");
				let old_port = c.get("firewall", params.id, "dest_port");
				if (old_enabled != "0") {
					if (old_ip != null) {
						if (type(old_ip) == "array") {
							for (let ip in old_ip)
								cmd = cmd ? append_clean_cmd(cmd, generate_clean_cmd(old_port, ip))
								          : generate_clean_cmd(old_port, ip);
						} else {
							cmd = generate_clean_cmd(old_port, old_ip);
						}
					} else {
						cmd = generate_clean_cmd(old_port);
					}
				}
				c.delete("firewall", params.id);
			}
		}
		c.commit("firewall");
		firewall_apply(ctx, params.apply, cmd);
		ctx.popen("/etc/init.d/firewall reload 2>/dev/null");
		return {};
	},

	order_acl_rule: function(args, ctx) {
		let params = args;
		let c = ctx.uci.cursor();
		if (type(params.id_list) != "array") return {};
		let rule_index = null;
		c.foreach("firewall", "rule", function(s) {
			if (s.name && substr(s.name, 0, 5) == CUSTOM_ACL_PREFIX && rule_index == null)
				rule_index = s[".index"];
		});
		let idx = 0;
		if (rule_index != null) {
			for (let id in params.id_list) {
				c.reorder("firewall", id, rule_index + idx);
				idx++;
			}
		}
		c.commit("firewall");
		firewall_apply(ctx, params.apply);
		ctx.popen("/etc/init.d/firewall reload 2>/dev/null");
		return {};
	},

	get_acl_zone_list: function(args, ctx) {
		let c = ctx.uci.cursor();
		return acl_zone_list(c);
	}
};
