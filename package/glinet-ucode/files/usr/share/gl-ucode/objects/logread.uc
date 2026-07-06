// gl-ucode object: logread  (port of decompiled-spec/rpc/logread.lua)
// System/kernel/crash log viewer. get_system_log shells busybox `logread` and
// filters in-code; get_kernel_log shells `dmesg`. DEGRADE: get_nginx_log and
// get_esim_log return {log:""} (mainline uhttpd, no GL nginx / no esim);
// export_logs builds a small tar via popen instead of GL /usr/bin/export_logs.
// get_crash_log/remove_crash_log keep only the /sys/fs/pstore branch; the
// GL mtd `oopslog` fallback is DROPPED (no GL log mtd on the SF1688).
'use strict';

const fs = require('fs');

const ERR_PARAMETER = -1;

const module_mapping = {
	modem: [ "modem", "sms_manager", "simo", "vsim", "at_cache" ],
	repeater: [ "repeater", "wpa_supplicant" ],
	wifi: [ "mac80211", "wifi", "hostapd" ],
	vpn: [ "vpn", "wg" ],
	ddns: [ "ddns" ],
	tethering: [ "tethering" ],
	ipv6: [ "nat6", "IPv6" ]
};

const no_wifi_module_mapping = {
	modem: [ "modem", "sms_manager", "simo", "vsim", "at_cache" ],
	vpn: [ "vpn", "wg" ],
	ddns: [ "ddns" ],
	tethering: [ "tethering" ],
	ipv6: [ "nat6", "IPv6" ]
};

// Parse one `logread` line and decide whether it passes the given filters.
// module_filter is an array of literal module tokens; keyword is a literal
// substring. (The original escaped filters to Lua-literal patterns; here we
// use literal index() searches, so no escaping is needed.)
function process_log_line(line, level_filter, module_filter, keyword_filter) {
	let m = match(line, /^([a-zA-Z]+ [a-zA-Z]+)\s*(\d+)\s*(\d+:\d+:\d+)\s*(\d+)\s*([a-zA-Z.]+)\s([a-zA-Z._\d-]+)\[?(\d*)\]?:?(.*)/);
	if (!m)
		return false;
	let level = m[5];
	let module = m[6];
	let message = m[8];

	if (!level_filter && length(module_filter) == 0 && !keyword_filter)
		return true;

	if (level_filter && level_filter != "all") {
		let filter_prefix = substr(level_filter, 0, 3);
		if (index(level, filter_prefix) < 0)
			return false;
	}

	if (module_filter && module_filter[0] && module_filter[0] != "all") {
		let module_matched = false;
		for (let mod in module_filter) {
			if (index(module, mod) >= 0 || index(message, mod) >= 0) {
				module_matched = true;
				break;
			}
		}
		if (!module_matched)
			return false;
	}

	if (keyword_filter && index(line, keyword_filter) < 0)
		return false;

	return true;
}

function check_wifi_exists() {
	let content = fs.readfile("/etc/config/wireless");
	return (content != null && length(content) > 1);
}

export default {
	get_system_log: function(args, ctx) {
		let lines = args.lines;
		let level = args.level;
		let module = args.module;
		let keyword = args.keyword;

		if (lines != null && ((type(lines) != "int" && type(lines) != "double") || lines < 0))
			return { err_code: ERR_PARAMETER, err_msg: "lines must be a non-negative number" };

		let log = ctx.popen("logread") ?? "";

		let module_list = [];
		if (type(module) == "array") {
			for (let value in module) {
				if (module_mapping[value]) {
					for (let v in module_mapping[value])
						push(module_list, v);
				} else {
					push(module_list, value);
				}
			}
		} else {
			module_list = [ module ];
		}

		let valid = [];
		for (let line in split(log, "\n")) {
			if (length(line) > 0 && process_log_line(line, level, module_list, keyword))
				push(valid, line);
		}

		// Keep the last `lines` entries (matches the original's rolling buffer,
		// which retains at least 1 line even when lines == 0).
		if (lines != null) {
			let keep = lines > 1 ? lines : 1;
			if (length(valid) > keep) {
				let trimmed = [];
				for (let i = length(valid) - keep; i < length(valid); i++)
					push(trimmed, valid[i]);
				valid = trimmed;
			}
		}

		return { log: join("\n", valid) };
	},

	get_module_name: function(args, ctx) {
		let mapping = check_wifi_exists() ? module_mapping : no_wifi_module_mapping;
		return { modules: keys(mapping) };
	},

	get_kernel_log: function(args, ctx) {
		return { log: ctx.popen("dmesg") ?? "" };
	},

	// DEGRADE: mainline uhttpd, no GL nginx access/error logs.
	get_nginx_log: function(args, ctx) {
		return { log: "" };
	},

	get_crash_log: function(args, ctx) {
		let mode = args.mode;
		let log_number = args.log_number;

		if (mode != null && type(mode) != "int" && type(mode) != "double")
			return { err_code: ERR_PARAMETER, err_msg: "mode is invalid" };
		if (log_number != null && type(log_number) != "int" && type(log_number) != "double")
			return { err_code: ERR_PARAMETER, err_msg: "log_number is invalid" };

		// mode is only meaningful for the dropped GL mtd oopslog fallback; on the
		// SF1688 crash logs come from /sys/fs/pstore (ramoops).
		if (fs.access("/sys/fs/pstore")) {
			let logs = [];
			if (log_number != null) {
				let d = fs.readfile(`/sys/fs/pstore/dmesg-ramoops-${log_number}`);
				if (d != null)
					push(logs, d);
			} else {
				for (let i = 0; i <= 10; i++) {
					let name = `/sys/fs/pstore/dmesg-ramoops-${i}`;
					if (fs.access(name)) {
						let d = fs.readfile(name);
						if (d != null)
							push(logs, d + "\n\n\n");
					}
				}
			}
			if (length(logs) > 0)
				return { log: join("", logs) };
			if (fs.access("/tmp/log/crash_log")) {
				let d = fs.readfile("/tmp/log/crash_log");
				return { log: (d ?? "") + "\n\n\n" };
			}
		}
		// DROPPED: GL mtd `oopslog` fallback (no GL log partition on this device).
		return { log: "" };
	},

	remove_crash_log: function(args, ctx) {
		if (fs.access("/sys/fs/pstore")) {
			for (let i = 0; i <= 10; i++) {
				let name = `/sys/fs/pstore/dmesg-ramoops-${i}`;
				if (fs.access(name))
					fs.unlink(name);
			}
		}
		if (fs.access("/tmp/log/crash_log"))
			fs.unlink("/tmp/log/crash_log");
		// DROPPED: GL mtd erase / blkid PARTLABEL="log" fallback.
		return {};
	},

	// DEGRADE: no GL /usr/bin/export_logs; assemble a small tar of system logs.
	export_logs: function(args, ctx) {
		let debug_cmd_en = args.debug_logs_en;
		ctx.popen("mkdir -p /tmp/log_export && logread > /tmp/log_export/system.log 2>&1; dmesg > /tmp/log_export/kernel.log 2>&1");
		if (debug_cmd_en)
			ctx.popen("dmesg -l debug > /tmp/log_export/debug.log 2>&1");
		ctx.popen("tar -czf /tmp/export_logs.tar.gz -C /tmp/log_export . 2>/dev/null");
		return {};
	},

	// DEGRADE: no esim subsystem on this device.
	get_esim_log: function(args, ctx) {
		return { log: "" };
	}
};
