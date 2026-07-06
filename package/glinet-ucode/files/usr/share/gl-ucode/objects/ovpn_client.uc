// gl-ucode object: ovpn_client  (port of decompiled-spec/rpc/ovpn_client.lua)
// Thin shim over the `ovpnclient` uci config: renames the client section whose
// group_id/client_id match, with a UTF-8-safe 64-char clamp. The full OpenVPN
// CRUD lives in a closed .so; only set_config_name was decompiled, so only it is
// ported here. DEGRADE: oui.fs.sync() -> ctx.popen("sync") (no fs.sync binding).
'use strict';

const fs = require('fs');

export default {
	// --- status (app polls this for the dashboard VPN tile) ----------------
	// The closed .so tracked the running openvpn client; here we report connected
	// only when an openvpn client process is alive and the tun iface exists.
	// Degrades to status:0 (disconnected) otherwise.
	get_status: function(params, ctx) {
		let running = ctx.popen("pgrep -f 'openvpn.*client' 2>/dev/null");
		let has_tun = ctx.popen("ip link show tun0 2>/dev/null");
		let status = (running && length(running) && has_tun && length(has_tun)) ? 1 : 0;
		return { status: status, name: "", rx_bytes: 0, tx_bytes: 0 };
	},

	set_config_name: function(params, ctx) {
		let res = {};
		let c = ctx.uci.cursor();

		// Lua nil-check (0 and "" are valid): fail only when a param is absent.
		if (params.group_id == null || params.client_id == null || params.name == null) {
			return {
				err_code: -10,
				err_msg: "parameter missing"
			};
		}

		// Clamp to 64 bytes without splitting a UTF-8 multi-byte sequence: walk
		// back off any trailing continuation byte (0x80-0xBF). ord(name, safe_len)
		// is the 0-indexed byte at position safe_len == Lua string.byte(name, safe_len+1).
		if (length(params.name) > 64) {
			let safe_len = 64;
			while (safe_len > 0) {
				let byte = ord(params.name, safe_len);
				if (byte != null && byte >= 128 && byte <= 191) {
					safe_len = safe_len - 1;
				} else {
					break;
				}
			}
			params.name = substr(params.name, 0, safe_len);
		}

		c.foreach("ovpnclient", "clients", function(s) {
			if (s.group_id == null || s.client_id == null) {
				return;
			}
			if (int(s.group_id) == int(params.group_id) && int(s.client_id) == int(params.client_id)) {
				c.set("ovpnclient", s[".name"], "name", params.name);
			}
		});

		c.commit("ovpnclient");
		ctx.popen("sync 2>/dev/null");   // mirror oui.fs.sync(): flush uci write to flash
		return res;
	}
};
