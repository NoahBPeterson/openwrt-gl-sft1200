// gl-ucode object: luci  (port of decompiled-spec/rpc/luci.lua)
// GL's on-demand LuCI install/uninstall shim. On mainline LuCI is present or
// apk-installable, so the GL dance is mostly moot.
// DEGRADE: dropped GL-only lfactory.get_model free-space table, df-overlay space
// check, /usr/bin/init_luci.sh + uninit_luci.sh process-tracking, and the
// uhttpd "127.0.0.1:8080" listen-string convention. Status is driven off package
// presence; install/uninstall wrap `apk add/del luci` and no-op gracefully when
// luci is already in the desired state or apk is unavailable.
'use strict';

const fs = require('fs');

// status enum (preserved from Lua): 0/1/2/3 = uninstalled/installing/installed/uninstalling
const UNINSTALLED  = 0;
const INSTALLING   = 1;
const INSTALLED    = 2;
const UNINSTALLING = 3;

function have_apk() {
	return fs.access("/usr/bin/apk") || fs.access("/sbin/apk");
}

// true if LuCI appears installed, driven off package/file presence (not the
// GL uhttpd listen-string convention).
function luci_present(ctx) {
	if (fs.access("/usr/lib/lua/luci") || fs.access("/www/luci-static"))
		return true;
	if (have_apk()) {
		let out = ctx.popen("apk info -e luci 2>/dev/null");
		if (out && length(trim(out)) > 0)
			return true;
	}
	return false;
}

export default {
	install_luci: function(args, ctx) {
		if (luci_present(ctx))
			return { status: INSTALLED };
		if (!have_apk())
			// no package manager: nothing to install against, degrade to success
			return { status: luci_present(ctx) ? INSTALLED : UNINSTALLED };
		// heavy network/install op is intentional here (a set-style method)
		ctx.popen("apk add luci luci-ssl >/dev/null 2>&1 &");
		return { status: INSTALLING };
	},

	get_status: function(args, ctx) {
		return { status: luci_present(ctx) ? INSTALLED : UNINSTALLED };
	},

	uninstall_luci: function(args, ctx) {
		if (!luci_present(ctx))
			return { status: UNINSTALLED };
		if (!have_apk())
			return { status: INSTALLED };
		ctx.popen("apk del luci luci-ssl >/dev/null 2>&1 &");
		return { status: UNINSTALLING };
	}
};
