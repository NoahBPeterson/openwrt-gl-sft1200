// gl-ucode object: adguardhome  (port of decompiled-spec/rpc/adguardhome.lua)
// AdGuard Home ad/DNS-blocking toggle. Backs up dnsmasq DNS settings, points the
// dnsmasq upstream at AGH on 127.0.0.1#3053 (+ noresolv/localuse), then start/stops
// AGH via /etc/init.d/adguardhome. Store + enforcement is mainline (uci `dhcp`,
// uci `adguardhome`, ubus `service`).
// DEGRADE:
//   - The GL multi-resolver backup/restore (gl-dns-v2 / stubby / dnscrypt-proxy /
//     dnsproxy) is reduced to the mainline dnsmasq backup/restore only; those extra
//     resolver packages don't exist on the Siflower device. The stubby/dnscrypt/
//     dnsproxy init.d stop/start calls are dropped with them.
//   - The vpn_client (set_dns_override_vpn) and TAP-S2S (ovpnclient) guard branches
//     are dropped (GL C helpers / configs absent).
//   - The /lib/wifi/qcawificfg80211.sh driver-fork check is dropped: Siflower is
//     mac80211, so we ALWAYS take the ubus `service` config.change path.
//   - If /etc/init.d/adguardhome or the `adguardhome` uci config is absent,
//     get_config returns {enabled:false, dns_enabled:false} and set_config still
//     writes the uci (so it takes effect once the pkg is installed).
'use strict';

const fs = require('fs');

// uci has no cursor.rename in this ctx; emulate. Back up `from` -> `to` only if the
// backup slot `to` is not already populated (mirrors the Lua `if not _old` guard).
function backup_opt(c, cfg, sec, src, dst) {
	if (c.get(cfg, sec, dst) != null) return;
	let v = c.get(cfg, sec, src);
	if (v != null) {
		c.set(cfg, sec, dst, v);
		c.delete(cfg, sec, src);
	}
}

// Restore `src_old` -> `dst` if a backup exists (mirrors the Lua restore branch).
function restore_opt(c, cfg, sec, src_old, dst) {
	let v = c.get(cfg, sec, src_old);
	if (v != null) {
		c.delete(cfg, sec, dst);
		c.set(cfg, sec, dst, v);
		c.delete(cfg, sec, src_old);
	}
}

function backup_dns_config(c) {
	backup_opt(c, "dhcp", "@dnsmasq[0]", "boguspriv", "boguspriv_old");
	backup_opt(c, "dhcp", "@dnsmasq[0]", "cachesize", "cachesize_old");
	backup_opt(c, "dhcp", "@dnsmasq[0]", "noresolv", "noresolv_old");
	backup_opt(c, "dhcp", "@dnsmasq[0]", "localuse", "localuse_old");
	backup_opt(c, "dhcp", "@dnsmasq[0]", "server", "server_old");
}

function restore_dns_config(c) {
	restore_opt(c, "dhcp", "@dnsmasq[0]", "boguspriv_old", "boguspriv");
	restore_opt(c, "dhcp", "@dnsmasq[0]", "cachesize_old", "cachesize");
	restore_opt(c, "dhcp", "@dnsmasq[0]", "noresolv_old", "noresolv");
	restore_opt(c, "dhcp", "@dnsmasq[0]", "localuse_old", "localuse");
	restore_opt(c, "dhcp", "@dnsmasq[0]", "server_old", "server");
}

export default {
	get_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		// adguardhome uci may be absent -> get returns null -> both false.
		return {
			enabled: c.get("adguardhome", "config", "enabled") == "1",
			dns_enabled: c.get("adguardhome", "config", "dns_enabled") == "1"
		};
	},

	set_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let enabled = args.enabled;
		let dns_enabled = args.dns_enabled;

		// DEGRADE: TAP-S2S (ovpnclient) guard dropped (GL config absent).

		if (type(enabled) != "bool")
			return ctx.util.E.INVALID_PARAMS;
		if (dns_enabled != null && type(dns_enabled) != "bool")
			return ctx.util.E.INVALID_PARAMS;

		let enable_old = c.get("adguardhome", "config", "enabled") == "1";

		if (enabled) {
			if (!fs.access("/etc/AdGuardHome/backup/"))
				ctx.popen("mkdir -p /etc/AdGuardHome/backup");
			if (!enable_old)
				backup_dns_config(c);
			c.delete("dhcp", "@dnsmasq[0]", "boguspriv");
			c.delete("dhcp", "@dnsmasq[0]", "cachesize");
			c.set("dhcp", "@dnsmasq[0]", "noresolv", "1");
			c.set("dhcp", "@dnsmasq[0]", "server", ["127.0.0.1#3053"]);
			ctx.popen("ln -sf \"/tmp/resolv.conf.d/resolv.conf.auto\" /tmp/resolv.conf");
			c.set("dhcp", "@dnsmasq[0]", "localuse", "0");
			// DEGRADE: gl-dns-v2 / stubby / dnscrypt-proxy sets + their init.d stops dropped.
			c.commit("dhcp");
		} else {
			c.delete("dhcp", "@dnsmasq[0]", "noresolv");
			c.delete("dhcp", "@dnsmasq[0]", "server");
			c.delete("dhcp", "@dnsmasq[0]", "localuse");
			restore_dns_config(c);
			// DEGRADE: gl-dns-v2 / stubby / dnscrypt-proxy restore dropped.
			c.commit("dhcp");
		}

		// Persist toggle state. Ensure the section exists so the write also works
		// on a fresh install where the `adguardhome` config file is not yet present.
		if (c.get("adguardhome", "config") == null)
			c.set("adguardhome", "config", "adguardhome");
		c.set("adguardhome", "config", "enabled", enabled ? "1" : "0");
		if (dns_enabled != null)
			c.set("adguardhome", "config", "dns_enabled", dns_enabled ? "1" : "0");
		c.commit("adguardhome");

		// DEGRADE: vpn_client.set_dns_override_vpn(override_vpn) dropped (GL helper absent).

		ctx.popen("sync");

		// DEGRADE: always the mac80211/ubus path (no qcawificfg80211.sh fork).
		ctx.ubus.call("service", "event", {
			type: "config.change",
			data: { package: "dhcp" }
		});

		// Original delayed this ~2s (after the dnsmasq reload). We run it inline; the
		// stubby/dnscrypt/dnsproxy restart branch is dropped (packages absent).
		if (fs.access("/etc/init.d/adguardhome")) {
			ctx.popen("/etc/init.d/adguardhome stop >/dev/null 2>&1");
			if (enabled)
				ctx.popen("/etc/init.d/adguardhome start >/dev/null 2>&1");
		}

		return {};
	}
};
