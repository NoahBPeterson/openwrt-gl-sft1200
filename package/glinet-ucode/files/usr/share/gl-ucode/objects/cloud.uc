// gl-ucode object: cloud  (port of decompiled-spec/rpc/cloud.lua)
// GoodCloud bind/status/remote-access. Thin RPC over the `gl-cloud` ubus daemon,
// the `gl-cloud` uci store (@cloud[0]: enable/server/token/bindtime/username/email/
// bindtype) + rtty uci, and the GoodCloud API (api.gl-inet.cn / api.goodcloud.xyz).
// GL-only bits degraded:
//   * oui.utils/oui.fs/oui.ubus/ngx -> ctx.popen / ctx.uci / ctx.ubus.
//   * gl.vpn_client.get_policy_group() (procd cgroup) -> DROPPED (no group on mainline).
//   * /proc/gl-hw-info/country_code -> ctx.popen fallback to uci wireless country.
// DEGRADE: when the `gl-cloud` ubus object is absent, get_config still returns a sane
//   "not bound" shape (uci reads yield nulls -> cloud_enable:false, token_status:0,
//   serverzones:[...], rtty_*:false). bind_device/rebind/unbind/get_bind_url return an
//   "unavailable" error shape until the gl-cloud daemon is running.
'use strict';

const fs = require('fs');

function readfile(p) { let d = fs.readfile(p); return d ? trim(d) : null; }

// gl-cloud ubus object present/answering?
function cloud_up(ctx) {
	return ctx.ubus.call("gl-cloud", "status") != null;
}

function mqtt_is_connected(ctx) {
	let status = ctx.ubus.call("gl-cloud", "status");
	return status && status.connected != 0;
}

// loop up to 10s waiting for the gl-cloud service to come up + connect.
function mqtt_connect_succeed(ctx) {
	for (let i = 1; i <= 10; i++) {
		let srvs = ctx.ubus.call("service", "list", { name: "gl-cloud" }) ?? {};
		if (srvs["gl-cloud"] && mqtt_is_connected(ctx))
			return true;
		ctx.popen("sleep 1");
	}
	return false;
}

// loop up to 10s waiting for the daemon to persist bindtime into uci.
function wait_write_bind_info(ctx) {
	for (let i = 1; i <= 10; i++) {
		let c = ctx.uci.cursor();
		if (c.get("gl-cloud", "@cloud[0]", "bindtime"))
			return;
		ctx.popen("sleep 1");
	}
}

// src_format 1: gslb server -> {api,url}. src_format 0: cloud url -> gslb server.
function switch_server_format(server, src_format) {
	if (!server) return null;
	let valid_url = {
		"gslb.gl-inet.cn":       { api: "https://api.gl-inet.cn",   url: "https://cloud.gl-inet.cn" },
		"gslb-cn.goodcloud.xyz": { api: "https://api.gl-inet.cn",   url: "https://cloud.gl-inet.cn" },
		"gslb-eu.goodcloud.xyz": { api: "https://api.goodcloud.xyz", url: "https://www.goodcloud.xyz" }
	};
	let gslb_format = 1;
	for (let k in valid_url) {
		let v = valid_url[k];
		if (src_format == gslb_format) {
			if (server == k) return v;
		} else if (server == v.url) {
			return k;
		}
	}
	return null;
}

function device_is_2b() {
	return readfile("/etc/firmware_type") == "2b";
}

// /proc/gl-hw-info/country_code with a uci-wireless fallback.
function country_code(ctx) {
	let cc = ctx.popen("cat /proc/gl-hw-info/country_code 2>/dev/null");
	if (cc && trim(cc) != "") return trim(cc);
	let c = ctx.uci.cursor();
	return c.get("wireless", "radio0", "country") ?? c.get("wireless", "@wifi-device[0]", "country") ?? "";
}

function get_config(ctx) {
	let c = ctx.uci.cursor();
	let res = {};
	let server = c.get("gl-cloud", "@cloud[0]", "server");
	res.cloud_enable = c.get("gl-cloud", "@cloud[0]", "enable") == "1";
	res.rtty_ssh = c.get("rtty", "general", "ssh_en") == "1";
	res.rtty_web = c.get("rtty", "general", "web_en") == "1";
	res.serverzones = [ "Europe", "America", "Asia Pacific" ];
	res.username = c.get("gl-cloud", "@cloud[0]", "username");
	res.email = c.get("gl-cloud", "@cloud[0]", "email");
	res.bindtime = c.get("gl-cloud", "@cloud[0]", "bindtime");
	res.token_invalid = c.get("gl-cloud", "@cloud[0]", "token") == "invalid";
	res.bindtype = c.get("gl-cloud", "@cloud[0]", "bindtype") ?? "";

	let token = c.get("gl-cloud", "@cloud[0]", "token");
	if (token == "invalid")
		res.token_status = 2;
	else if (!mqtt_is_connected(ctx))     // DEGRADE: gl-cloud absent -> 0 ("not bound")
		res.token_status = 0;
	else
		res.token_status = 1;

	let srv_url = c.get("gl-cloud", "@cloud[0]", "srv_url");
	let srv_api = c.get("gl-cloud", "@cloud[0]", "srv_api");
	if (srv_url) {
		res.cloud_url = srv_url;
		res.api_url = srv_api;
	} else {
		let cloud_info = switch_server_format(server, 1);
		if (type(cloud_info) == "object") {
			res.cloud_url = cloud_info.url;
			res.api_url = cloud_info.api;
		}
	}

	let dev_opt = c.get("mptun", "global", "dev_opt");
	if (dev_opt == "dev") {
		res.aw_url = "https://cloud-sdwan-dev.gl-inet.cn";
		res.aw_api_url = res.aw_url;
	} else if (dev_opt == "test") {
		res.aw_url = "https://aw-test.gl-inet.cn";
		res.aw_api_url = res.aw_url;
	} else {
		res.aw_url = "https://my.astrowarp.net";
		res.aw_api_url = "https://api.astrowarp.net";
	}
	return res;
}

// resolve the GoodCloud API base url from srv_api or the gslb server map.
function api_url(ctx) {
	let c = ctx.uci.cursor();
	let srv_api = c.get("gl-cloud", "@cloud[0]", "srv_api");
	if (srv_api) return srv_api;
	let cloud_info = switch_server_format(c.get("gl-cloud", "@cloud[0]", "server"), 1);
	return (type(cloud_info) == "object") ? cloud_info.api : null;
}

export default {
	get_config: function(args, ctx) {
		return get_config(ctx);
	},

	set_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let cloud_enable = args.cloud_enable;
		let server = args.server_name;
		let rtty_ssh = args.rtty_ssh;
		let rtty_web = args.rtty_web;
		let url = args.url;

		c.set("gl-cloud", "@cloud[0]", "custom_enable", "1");
		let custom_srv = c.get("gl-cloud", "@cloud[0]", "srv_gslb");
		if (!custom_srv) {
			server = switch_server_format(url, 0);
			c.delete("gl-cloud", "@cloud[0]", "custom_enable");
			c.delete("gl-cloud", "@cloud[0]", "srv_url");
			c.delete("gl-cloud", "@cloud[0]", "srv_api");
		} else {
			server = custom_srv;
		}

		let need_restart_cloud = false;
		if (type(cloud_enable) == "bool")
			c.set("gl-cloud", "@cloud[0]", "enable", cloud_enable ? "1" : "0");
		if (type(rtty_ssh) == "bool") {
			c.set("rtty", "general", "ssh_en", rtty_ssh ? "1" : "0");
			need_restart_cloud = true;
		}
		if (type(rtty_web) == "bool") {
			c.set("rtty", "general", "web_en", rtty_web ? "1" : "0");
			need_restart_cloud = true;
		}
		if (args.clear_token && c.get("gl-cloud", "@cloud[0]", "token")) {
			c.delete("gl-cloud", "@cloud[0]", "token");
			need_restart_cloud = true;
		}

		let old_server = c.get("gl-cloud", "@cloud[0]", "server");
		if (server && !old_server) {
			c.set("gl-cloud", "@cloud[0]", "server", server);
			need_restart_cloud = true;
		}
		if (device_is_2b())
			c.set("gl-cloud", "@cloud[0]", "templete", args.templete ?? "1");

		cloud_enable = c.get("gl-cloud", "@cloud[0]", "enable") == "1";
		if (!cloud_enable)
			c.delete("gl-cloud", "@cloud[0]", "bindtime");

		c.commit("rtty");
		c.commit("gl-cloud");
		ctx.popen("sync");

		let resp = get_config(ctx);
		if (resp)
			ctx.ubus.call("gl-session", "notify", { name: "cloud.status", data: resp });

		if (!cloud_enable || need_restart_cloud) {
			ctx.ubus.call("service", "delete", { name: "rtty" });
			ctx.ubus.call("service", "delete", { name: "gl-cloud" });
			if (cloud_enable)
				ctx.popen("sleep 0.5");
		}

		if (cloud_enable) {
			let srvs = ctx.ubus.call("service", "list", { name: "gl-cloud" }) ?? {};
			if (!srvs["gl-cloud"] || !srvs["gl-cloud"].instances) {
				// DEGRADE: vpn_client policy group dropped (no cgroup on mainline).
				ctx.ubus.call("service", "add", {
					name: "gl-cloud",
					instances: {
						instance1: { command: [ "/usr/bin/eco", "/usr/bin/gl-cloud" ] }
					}
				});
				ctx.popen("/etc/init.d/gl-cloud restart >/dev/null 2>&1 &");
			}
		} else {
			if (fs.access("/etc/config/mptun")) {
				if (c.get("mptun", "global", "enable") == "1") {
					c.set("mptun", "global", "enable", "0");
					c.commit("mptun");
					ctx.popen("sync");
					ctx.popen("/etc/init.d/mptun stop >/dev/null 2>&1 &");
				}
				ctx.popen("/etc/init.d/mpifd stop >/dev/null 2>&1 &");
			}
			ctx.popen("/etc/init.d/gl-cloud stop >/dev/null 2>&1 &");
		}
		return {};
	},

	bind_device: function(args, ctx) {
		// DEGRADE: gl-cloud daemon absent -> unavailable (works once it runs).
		if (!cloud_up(ctx))
			return { err_code: -11, err_msg: "gl-cloud service unavailable" };

		let token = args.token;
		let org_id = args.org_id;
		let var_id = args.var_id;

		let url = api_url(ctx);
		if (!url)
			return { err_code: -9, err_msg: "url message error." };

		let id = "";
		if (!org_id && !var_id)
			url = url + "/cloud-api/cloud/v2/sdk/device/bind/self";
		if (org_id) {
			url = url + "/cloud-api/cloud/v2/sdk/device/bind/org";
			id = org_id;
		}
		if (var_id) {
			id = var_id;
			url = url + "/cloud-api/cloud/v2/sdk/device/bind/inventory";
		}

		if (!mqtt_is_connected(ctx)) {
			ctx.ubus.call("gl-cloud", "reconnect");
			if (!mqtt_connect_succeed(ctx))
				return { err_code: -10, err_msg: "mqtt disconnect" };
		}

		let ret = ctx.ubus.call("gl-cloud", "bind", { url: url, token: token, id: id });
		if (ret && ret.err)
			return { err_code: ret.code, err_msg: ret.err };

		ctx.ubus.call("gl-cloud", "alive");
		wait_write_bind_info(ctx);
		return {};
	},

	rebind: function(args, ctx) {
		if (!cloud_up(ctx))
			return { err_code: -11, err_msg: "gl-cloud service unavailable" };

		let token = args.token;
		let c = ctx.uci.cursor();

		let url = api_url(ctx);
		if (!url)
			return { err_code: -9, err_msg: "url message error." };

		let ret = ctx.ubus.call("gl-cloud", "rebind", { url: url, token: token });
		if (ret && ret.err)
			return { err_code: ret.code, err_msg: ret.err };

		let invalid = c.get("gl-cloud", "cloud[0]", "token");
		if (invalid && invalid == "invalid") {
			c.delete("gl-cloud", "cloud[0]", "token");
			c.commit("gl-cloud");
			ctx.popen("sync");
		}

		if (!mqtt_is_connected(ctx)) {
			ctx.ubus.call("gl-cloud", "reconnect");
			if (!mqtt_connect_succeed(ctx))
				return { err_code: -10, err_msg: "mqtt disconnect" };
		}

		ctx.ubus.call("gl-cloud", "alive");
		wait_write_bind_info(ctx);
		return {};
	},

	bind_info: function(args, ctx) {
		let username = args.username ?? "";
		let email = args.email ?? "";
		let bindtime = args.bindtime ?? "";
		if (bindtime == "")
			ctx.popen("/etc/init.d/rtty stop >/dev/null 2>&1");

		let c = ctx.uci.cursor();
		c.set("gl-cloud", "@cloud[0]", "username", username);
		c.set("gl-cloud", "@cloud[0]", "email", email);
		c.set("gl-cloud", "@cloud[0]", "bindtime", bindtime);
		c.commit("gl-cloud");
		ctx.popen("sync");
		return {};
	},

	unbind: function(args, ctx) {
		if (!cloud_up(ctx))
			return { err_code: -11, err_msg: "gl-cloud service unavailable" };

		let res = ctx.ubus.call("gl-cloud", "unbind") ?? {};
		if (res.code != 0)
			return { err_code: -1, err_msg: "unbind fail" };

		ctx.popen("sleep 3");
		let c = ctx.uci.cursor();
		c.delete("gl-cloud", "@cloud[0]", "username");
		c.delete("gl-cloud", "@cloud[0]", "email");
		c.delete("gl-cloud", "@cloud[0]", "bindtime");
		c.delete("gl-cloud", "@cloud[0]", "bindtype");

		if (fs.access("/etc/config/mptun")) {
			c.set("mptun", "global", "enable", "0");
			c.commit("mptun");
			ctx.popen("/etc/init.d/mptun stop >/dev/null 2>&1");
			ctx.popen("/etc/init.d/mpifd stop >/dev/null 2>&1");
		}

		let server_chg = false;
		if (country_code(ctx) == "CN") {
			let server = c.get("gl-cloud", "@cloud[0]", "server");
			if (server != "gslb.gl-inet.cn" && server != "gslb-cn.goodcloud.xyz") {
				c.set("gl-cloud", "@cloud[0]", "server", "gslb.gl-inet.cn");
				server_chg = true;
			}
		}
		c.commit("gl-cloud");
		ctx.popen("/etc/init.d/rtty stop >/dev/null 2>&1");
		if (server_chg)
			ctx.ubus.call("gl-cloud", "reconnect");
		ctx.popen("sync");
		return {};
	},

	get_bind_url: function(args, ctx) {
		if (!cloud_up(ctx))
			return { err_code: -11, err_msg: "gl-cloud service unavailable" };

		let c = ctx.uci.cursor();
		let res = {};
		let bindtime = c.get("gl-cloud", "@cloud[0]", "bindtime") ?? "";

		let resp = ctx.ubus.call("gl-cloud", "bind_url");
		if (!resp)
			return { err_code: -1, err_msg: "get bind url failed" };

		let url = resp.url ?? "";
		if (resp.err || url == "")
			return { err_code: resp.err_code, err_msg: resp.err };

		let isbind = (bindtime == "") ? 0 : 1;
		if (resp.err_code == 1) {
			res.rebind = true;
			isbind = 1;
		}
		res.url = sprintf("%s&isbind=%d", url, isbind);
		return res;
	}
};
