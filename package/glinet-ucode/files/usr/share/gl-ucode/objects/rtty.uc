// gl-ucode object: rtty  (port of decompiled-spec/rpc/rtty.lua)
// Remote web-terminal over GoodCloud. Reimplemented over the stock/OSS `rtty` daemon
// managed by procd (ubus service add/list/delete), pointed at the caller-supplied
// broker host/port, with the GoodCloud CA fetched from ca.gl-inet.com (or ca.gl-inet.cn
// in CN). rtty uci is the store.
// GL-only bits degraded:
//   * lfactory.get_country_code() -> ctx.popen /proc/gl-hw-info/country_code fallback.
//   * gl.vpn_client policy group (sudo -g <group>) -> DROPPED (no cgroup on mainline).
//   * /proc/gl-hw-info/device_ddns device id -> that proc file, else rtty.general.id,
//     else the system hostname.
// DEGRADE: get_config/set_config are store-only and always work. run/stop need the rtty
//   daemon: run returns {err_code:4,...} "unavailable" when /usr/sbin/rtty is absent.
'use strict';

const fs = require('fs');

const CA_PATH = "/tmp/ca.crt";

function readfile(p) { let d = fs.readfile(p); return d ? trim(d) : null; }

function config(ctx) {
	let c = ctx.uci.cursor();
	return {
		ssh_enabled: c.get("rtty", "general", "ssh_en") == "1",
		web_enabled: c.get("rtty", "general", "web_en") == "1"
	};
}

// /proc/gl-hw-info/country_code with a uci-wireless fallback.
function country_code(ctx) {
	let cc = ctx.popen("cat /proc/gl-hw-info/country_code 2>/dev/null");
	if (cc && trim(cc) != "") return trim(cc);
	let c = ctx.uci.cursor();
	return c.get("wireless", "radio0", "country") ?? c.get("wireless", "@wifi-device[0]", "country") ?? "";
}

// rtty device id: GL ddns proc file, else uci rtty.general.id, else hostname.
function device_id(ctx) {
	return readfile("/proc/gl-hw-info/device_ddns")
	    ?? ctx.uci.cursor().get("rtty", "general", "id")
	    ?? readfile("/proc/sys/kernel/hostname");
}

function rtty_running(ctx) {
	let s = ctx.ubus.call("service", "list", { name: "rtty" });
	return s && s.rtty && s.rtty.instances && s.rtty.instances.instance1 && s.rtty.instances.instance1.running;
}

export default {
	get_config: function(args, ctx) {
		return config(ctx);
	},

	set_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		c.set("rtty", "general", "ssh_en", args.ssh_enable ? "1" : "0");
		c.set("rtty", "general", "web_en", args.web_enable ? "1" : "0");
		c.commit("rtty");
		ctx.popen("sync");
		ctx.ubus.call("gl-cloud", "alive");   // DEGRADE: no-op if gl-cloud absent
		return config(ctx);
	},

	run: function(args, ctx) {
		let token = args.token;
		let host = args.host;
		let port = args.port;
		let ssl = args.ssl ?? false;

		if (type(host) != "string" || (type(port) != "int" && type(port) != "double"))
			return ctx.util.E.INVALID_PARAMS;

		// DEGRADE: stock rtty daemon not installed -> unavailable.
		if (!fs.access("/usr/sbin/rtty"))
			return { err_code: 4, err_msg: "rtty daemon not installed" };

		let want = `${host}${port}${ssl ? "true" : "false"}`;
		let last_host_port = readfile("/tmp/rtty-host-port");
		if (last_host_port != want)
			ctx.ubus.call("service", "delete", { name: "rtty" });

		if (rtty_running(ctx))
			return {};

		let ddns = device_id(ctx);
		if (!ddns || match(ddns, /[^a-zA-Z0-9_-]/))
			return { err_code: 1, err_msg: "no valid ddns" };

		let c = ctx.uci.cursor();
		let http_timeout = int(c.get("rtty", "general", "http_timeout") ?? "0") || 180;

		// DEGRADE: vpn_client policy group (sudo -g <group>) dropped on mainline.
		let command = [
			"/usr/sbin/rtty",
			"-I", ddns,
			"-h", host,
			"-p", `${port}`,
			"--http-timeout", `${http_timeout}`
		];
		if (token) { push(command, "-t"); push(command, token); }
		if (ssl) push(command, "-s");

		if (!fs.access(CA_PATH)) {
			let ca_host = (country_code(ctx) == "CN") ? "ca.gl-inet.cn" : "ca.gl-inet.com";
			ctx.popen(`wget -O ${CA_PATH} https://${ca_host}/goodcloud/ca.crt 2>/dev/null`);
		}
		if (!fs.access(CA_PATH))
			return { err_code: 3, err_msg: "failed to get ca certificate" };

		push(command, "-C");
		push(command, CA_PATH);

		ctx.ubus.call("service", "add", {
			name: "rtty",
			instances: { instance1: { command: command } }
		});

		ctx.popen("sleep 0.5");
		for (let i = 0; i < 5; i++) {
			if (rtty_running(ctx)) {
				fs.writefile("/tmp/rtty-host-port", want);
				return {};
			}
			ctx.popen("sleep 0.5");
		}
		return { err_code: 2, err_msg: "connect failed" };
	},

	stop: function(args, ctx) {
		ctx.ubus.call("service", "delete", { name: "rtty" });
		return {};
	}
};
