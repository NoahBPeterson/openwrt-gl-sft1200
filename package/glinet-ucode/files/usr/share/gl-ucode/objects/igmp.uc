// gl-ucode object: igmp  (port of decompiled-spec/rpc/igmp.lua)
// IGMP snooping toggle + force_igmp_version. Modern (>=21, device-model) path only:
// snooping is a bridge `device` uci option on the br-lan device section in `network`.
// DEGRADE: dropped all mtk/mtkwifi/mwctl/mcsd/e5800 wireless branches (Siflower
// mac80211 handles snooping at the bridge) and the OpenWrt<21 ifname path.
'use strict';

const fs = require('fs');

// Read igmp_snooping for a logical interface off its bridge `device` section
// (device-model path); fall back to the option on the interface itself.
function get_snooping(c, iface) {
	let device = c.get("network", iface, "device");
	if (device == null)
		return c.get("network", iface, "igmp_snooping") == "1";
	let enable = false;
	c.foreach("network", "device", function(s) {
		if (s.name == device)
			enable = (c.get("network", s[".name"], "igmp_snooping") == "1");
	});
	return enable;
}

// Set igmp_snooping for a logical interface on its bridge `device` section.
function set_snooping(c, iface, val) {
	let device = c.get("network", iface, "device");
	if (device == null) {
		c.set("network", iface, "igmp_snooping", val);
		return;
	}
	c.foreach("network", "device", function(s) {
		if (s.name == device)
			c.set("network", s[".name"], "igmp_snooping", val);
	});
}

export default {
	get_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let data = fs.readfile("/etc/sysctl.conf") ?? "";
		let m = match(data, /force_igmp_version=([0-9]+)/);
		return {
			enable: get_snooping(c, "lan"),
			version: m ? int(m[1]) : 3
		};
	},

	set_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		if (type(args.enable) != "bool")
			return { err_code: -1, err_msg: "enable error" };
		if (args.version < 1 || args.version > 3)
			return { err_code: -1, err_msg: "version error" };

		let val = args.enable ? 1 : 0;
		for (let iface in ["lan", "guest", "iot"])
			set_snooping(c, iface, val);
		c.commit("network");

		// Rewrite /etc/sysctl.conf, replacing any force_igmp_version line.
		let kept = [];
		for (let l in split(fs.readfile("/etc/sysctl.conf") ?? "", "\n"))
			if (index(l, "force_igmp_version") < 0)
				push(kept, l);
		let body = trim(join(kept, "\n"));
		let content = (body != "" ? body + "\n" : "") +
			`net.ipv4.conf.all.force_igmp_version=${args.version}\n`;
		fs.writefile("/etc/sysctl.conf", content);

		ctx.ubus.call("network", "reload");
		ctx.popen("/etc/init.d/sysctl reload >/dev/null 2>&1");
		return {};
	}
};
