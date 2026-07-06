// gl-ucode object: ddns  (app's DDNS page -> ddns.get_config / ddns.get_status)
// Stock object is a closed gl-sdk4-ddns .so over the `gl_ddns` uci + GoodCloud
// DDNS hostname (a 32-hex label under the vendor's ddns domain, stored in the
// factory partition). We report uci-backed config + the factory ddns host and
// degrade the cloud-registration status to "not running" when the daemon's absent.
'use strict';

const fs = require('fs');

export default {
	get_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let enabled = (c.get("gl_ddns", "glddns", "enabled") == "1");
		let host = c.get("factory", "device", "ddns") ?? "";
		let domain = host ? `${host}.glddns.com` : "";
		return {
			enabled: enabled,
			host_name: host,
			domain: domain,
			interface: c.get("gl_ddns", "glddns", "interface") ?? "wan",
			ipv6_enabled: (c.get("gl_ddns", "glddnsv6", "enabled") == "1"),
			ddns_analysis: domain
		};
	},

	get_status: function(args, ctx) {
		let c = ctx.uci.cursor();
		let enabled = (c.get("gl_ddns", "glddns", "enabled") == "1");
		// 0 = off, 1 = running/registered, 2 = enabled-but-not-yet-online. The cloud
		// DDNS daemon is absent on mainline, so a live host reads as "enabled/pending".
		let running = length(ctx.popen("pgrep -f gl_ddns 2>/dev/null") ?? "") > 0;
		return { enabled: enabled, status: enabled ? (running ? 1 : 2) : 0 };
	}
};
