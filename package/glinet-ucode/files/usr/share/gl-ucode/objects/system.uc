// gl-ucode object: system  (port of decompiled-spec/rpc/system.lua)
// v1 = launch/dashboard subset. GL-only sources (lfactory, /proc/gl-hw-info,
// glconfig, kmwan, hardware) are replaced with mainline ubus/uci/procfs and
// degrade gracefully (same field names + sane defaults as the original).
'use strict';

const fs = require('fs');

function readfile(p) { let d = fs.readfile(p); return d ? trim(d) : null; }

function os_version(ctx) {
	let b = ctx.ubus.call("system", "board") ?? {};
	let v = (b.release && b.release.version) ? b.release.version : "";
	let m = match(v, /([0-9]+)/);
	return m ? int(m[1]) : 0;
}

function model(ctx) {
	let c = ctx.uci.cursor();
	return readfile("/proc/gl-hw-info/model")
	    ?? c.get("board_special", "hardware", "model")
	    ?? "sft1200";
}

function mac(ctx) {
	for (let dev in ["br-lan", "eth0", "eth0.1"]) {
		let a = readfile(`/sys/class/net/${dev}/address`);
		if (a) return uc(a);
	}
	return "";
}

export default {
	get_unixtime: function(args, ctx) {
		return { unixtime: time() };
	},

	get_model: function(args, ctx) {
		return { model: model(ctx) };
	},

	get_mac: function(args, ctx) {
		return { mac: mac(ctx) };
	},

	get_firmware_version: function(args, ctx) {
		let b = ctx.ubus.call("system", "board") ?? {};
		return {
			firmware_version: (b.release && b.release.version) ? b.release.version : "",
			firmware_description: (b.release && b.release.description) ? b.release.description : ""
		};
	},

	get_firmware_date: function(args, ctx) {
		return { firmware_date: readfile("/etc/version.date") ?? readfile("/etc/openwrt_version") ?? "" };
	},

	// Full V3 shape (matches gl-sdk4-system get_info): model, mac, vendor,
	// hardware/firmware version, firmware_date/type, ddns, sn, country_code, and the
	// hardware_feature/software_feature/board_info sub-objects the app's Firmware +
	// device pages read. GL-only sources degrade to mainline board/uci/factory data.
	get_info: function(args, ctx) {
		let c = ctx.uci.cursor();
		let b = ctx.ubus.call("system", "board") ?? {};
		let m = mac(ctx);
		// The app gates its whole API (V2 vs V3 response shapes) on this version via
		// RouterStatus.isV3() == Version(fw) < "4.0.0". Our real OpenWrt version
		// "25.12-SNAPSHOT" fails the app's strict semver parse and falls below 4.0.0,
		// forcing the V3 parser — whose nested mem{}/flash{}/loadavg shape our V2-style
		// ports don't match (→ every V3 page throws & blanks). Report a clean GL-style
		// version >= 4.0.0 so the app uses the V2 API that matches every ported object.
		let real_fw = (b.release && b.release.version) ? b.release.version : "";
		let fw = "4.8.3";
		return {
			model: model(ctx),
			mac: m,
			factory_mac: m,
			macaddr: m,
			vendor: "GL.iNet",
			version: os_version(ctx),
			hardware_version: b.board_name ?? "",
			firmware_version: fw,
			firmware_date: readfile("/etc/version.date") ?? readfile("/etc/openwrt_version") ?? "",
			firmware_type: readfile("/etc/version.type") ?? readfile("/etc/firmware_type") ?? "release",
			kernel: b.kernel ?? "",
			board_name: b.board_name ?? "",
			ddns: c.get("factory", "device", "ddns") ?? "",
			sn: c.get("factory", "device", "sn") ?? "",
			sn_bak: "",
			country_code: c.get("factory", "device", "country") ?? "US",
			cpu_num: (function(){ let n = int(ctx.popen("grep -c ^processor /proc/cpuinfo 2>/dev/null") ?? "1"); return n < 1 ? 1 : n; })(),
			hardware_feature: {
				wan: "eth0.2", lan: "eth0.1", usb: "", radio: "dual",
				reset_button: "1", switch_button: "0", microsd: "0",
				mcu: false, fan: false, nand: true
			},
			// The app gates whole feature areas on these flags (real system.lua builds
			// them from fs.access on the gl .so's). We reimplement those objects in
			// ucode, so report support for what we actually serve. `vpn: true` is what
			// unlocks the WireGuard/OpenVPN client pages (else only the cloud wizard shows).
			software_feature: {
				ipv6: true,
				vpn: true,                                              // wg-client + ovpn-client + vpn-client
				adguard: fs.access("/usr/bin/AdGuardHome") ? true : false,
				tor: false,
				ids_ips: false,
				sms_forward: false,
				repeater_eap: true,                                     // mac80211 STA (EAP-capable)
				nas: fs.access("/usr/sbin/ksmbd.mountd") ? true : false,
				passthrough: false,
				secondwan: false,
				bark: false,
				mlo: false,
				cellular_upgrade: false,
				ksmbd: fs.access("/usr/sbin/ksmbd.mountd") ? true : false,
				astrowarp_lite: false,
				obfuscation: false,
				minimum_temperature: "70"
			},
			board_info: { model: model(ctx), firmware_version: fw }
		};
	},

	get_load: function(args, ctx) {
		let m = match(readfile("/proc/loadavg") ?? "", /^([0-9.]+) ([0-9.]+) ([0-9.]+)/);
		return { load_average: m ? [ +m[1], +m[2], +m[3] ] : [0, 0, 0] };
	},

	// Dashboard payload. Subset of the original's fields, mainline-sourced.
	get_status: function(args, ctx) {
		let c = ctx.uci.cursor();
		let info = ctx.ubus.call("system", "info") ?? {};
		let mem = info.memory ?? {};
		let m = match(readfile("/proc/loadavg") ?? "", /^([0-9.]+) ([0-9.]+) ([0-9.]+)/);
		let up = match(readfile("/proc/uptime") ?? "", /^([0-9.]+)/);

		// flash usage (the app's flash tile reads flash_total/flash_app/flash_free).
		// This ucode build has no fs.statvfs, so parse `df` on the overlay (the writable
		// root); fall back to / when there's no overlay mount. df -k reports 1K blocks.
		// df -k columns: <blocks> <used> <available> <use%>. Capture blocks + available.
		let flash_total = 0, flash_free = 0;
		let dfout = ctx.popen("df -k /overlay 2>/dev/null | tail -1 || df -k / | tail -1");
		let dm = match(dfout ?? "", /([0-9]+)[[:space:]]+([0-9]+)[[:space:]]+([0-9]+)[[:space:]]+[0-9]+%/);
		if (dm) { flash_total = int(dm[1]) * 1024; flash_free = int(dm[3]) * 1024; }

		// cpu: real firmware get_status returns `cpu = math.floor(<usage%>)` — a SCALAR,
		// not an object. It's an integer 0-100 from a short /proc/stat delta.
		let cpu = int(+(ctx.popen(
			"A=$(awk '/^cpu /{print $2+$3+$4+$5+$6+$7+$8,$5+$6;exit}' /proc/stat); " +
			"sleep 0.25; " +
			"B=$(awk '/^cpu /{print $2+$3+$4+$5+$6+$7+$8,$5+$6;exit}' /proc/stat); " +
			"echo $A $B | awk '{dt=$3-$1;di=$4-$2;if(dt<=0)printf\"0\";else printf\"%.0f\",(dt-di)*100/dt}'"
		) ?? "0"));

		// tzoffset: the real handler's get_tzoffset() returns the RAW STRING from
		// `date +'%z'` (e.g. "+0000"). The app parses tzoffset as String? — sending an
		// int throws the whole System model. Port it faithfully: keep it a string.
		let tzoffset = ctx.popen("date +'%z' 2>/dev/null") ?? "+0000";

		// lan ipaddr may be a uci list with a CIDR suffix; present a bare ip + netmask
		let lanip_raw = c.get("network", "lan", "ipaddr");
		if (type(lanip_raw) == "array") lanip_raw = lanip_raw[0];
		lanip_raw ??= "";
		let lanip = replace(lanip_raw, /\/.*$/, "");
		let lanmask = c.get("network", "lan", "netmask")
			?? (match(lanip_raw, /\/24$/) ? "255.255.255.0" : "");

		// netnat (hw flow offload) enabled?
		let netnat = (c.get("firewall", "@defaults[0]", "flow_offloading_hw") == "1")
			|| (c.get("firewall", "@defaults[0]", "flow_offloading") == "1");

		// qos_enabled: any qos.queue section present (matches the Lua foreach)
		let qos_enabled = false;
		let qall = c.get_all("qos");
		if (qall) for (let k in qall) if (qall[k][".type"] == "queue") { qos_enabled = true; break; }

		// CRITICAL STRUCTURE (confirmed via Blutter decompile of SystemStatusM.fromJson +
		// the real 4.8.3 handler): the dashboard fields (cpu/mem/flash/lan_ip/...) are
		// NESTED under a "system" key; `network`/`wifi`/`service`/`client`/`mcu` are its
		// SIBLINGS. The app does `System.fromJson(json["system"])`, so a flat top-level
		// payload leaves json["system"] == null -> the whole tile shows its zero default
		// (CPU 0 / mem 100% / flash 0). These 20 fields are exactly System.fromJson's.
		let cpu_num = (function(){ let n = int(ctx.popen("grep -c ^processor /proc/cpuinfo 2>/dev/null") ?? "1"); return n < 1 ? 1 : n; })();
		// The app's System model parses `cpu` as a NESTED object (Cpu{temperature:int?}),
		// NOT a scalar — sending a bare int fails its `as Map<String,dynamic>` cast and
		// throws ALL of System.fromJson, nulling every tile (CPU/mem/flash). Read the SoC
		// thermal zone (millidegrees -> °C) when present, else 0.
		let cpu_temp = int(int(readfile("/sys/class/thermal/thermal_zone0/temp") ?? "0") / 1000);
		let system = {
			mode: 0,                       // router (glconfig.general.mode absent -> default)
			timestamp: time(),
			tzoffset: tzoffset,
			uptime: up ? +up[1] : 0,
			cpu: { temperature: cpu_temp },
			cpu_usage: cpu,                // scalar usage% kept as a separate field
			cpu_num: cpu_num,
			load_average: m ? [ +m[1], +m[2], +m[3] ] : [0, 0, 0],
			memory_total: mem.total ?? 0,
			memory_free: mem.free ?? 0,
			memory_buff_cache: (mem.buffered ?? 0) + (mem.cached ?? 0),
			flash_total: flash_total,
			flash_free: flash_free,
			flash_app: flash_total - flash_free,
			lan_ip: lanip,
			lan_netmask: lanmask,
			guest_ip: c.get("network", "guest", "ipaddr") ?? "",
			guest_netmask: c.get("network", "guest", "netmask") ?? "",
			iot_ip: c.get("network", "iot", "ipaddr") ?? "",
			temperature: 0,
			ipv6_enabled: (c.get("glipv6", "globals", "enabled") == "1"),
			ddns_enabled: (c.get("gl_ddns", "glddns", "enabled") == "1"),
			netnat_enabled: netnat,
			prio_enabled: fs.access("/etc/config/gl_dpi_qos") ? (c.get("gl_dpi_qos", "qos", "prio_enable") == "1") : false,
			sqm_enabled: fs.access("/etc/config/sqm") ? (c.get("sqm", "@queue[0]", "enabled") == "1") : false,
			qos_enabled: qos_enabled,
			content_protection_enabled: fs.access("/etc/config/gl_dpi_content_protection") ? (c.get("gl_dpi_content_protection", "content_protection", "enabled") == "1") : false,
			flow_statistics_enabled: fs.access("/etc/config/gl_dpi_flow_statistics") ? (c.get("gl_dpi_flow_statistics", "global", "enable") == "1") : false,
			time_sync_status: fs.access("/var/state/ntp-valid") ? true : false,
			subnets: [],
			vlan_enable: (c.get("vlan", "config", "enable") == "1")
		};
		// The V2 (`System.fromJson(json["system"])`) and V3 (`fromJsonV3`, flat) models
		// are BOTH nullable-tolerant — a wrong shape doesn't throw, it just reads null →
		// the tile shows its zero default (CPU 0 / mem 100% / flash 0). So which parser
		// the app picks (isV3) matters, and we can't fully pin it — provide BOTH shapes:
		// V2 under `system`, V3 flat (mem/flash objects, loadavg, ip_addr, ...).
		let loadavg = m ? [ +m[1], +m[2], +m[3] ] : [0, 0, 0];
		let status = {
			system: system,
			// V3 flat shape (fromJsonV3)
			mem: { mem_total: mem.total ?? 0, mem_free: mem.free ?? 0,
			       mem_buffers: (mem.buffered ?? 0) + (mem.cached ?? 0) },
			flash: { flash_total: flash_total, flash_free: flash_free },
			loadavg: loadavg,
			cpu: cpu,
			mode: 0,
			ip_addr: lanip,
			guest_addr: c.get("network", "guest", "ipaddr") ?? "",
			ipv6info: { enable: (c.get("glipv6", "globals", "enabled") == "1") },
			wireless: [],
			// shared siblings. NOTE: `client` is an ARRAY of one status obj (the real
			// handler does `table.insert(client, gl-clients.status)`), not an object.
			service: [],
			wifi: [],
			client: [ { cable_total: 0, wireless_total: 0 } ],
			network: []
		};

		// per-interface up/online/proto from mainline netifd. The Home "internet" tile
		// keys off a wan-side interface being online; our uplink may be the ethernet WAN
		// OR the repeater STA (network 'internet'), so include whichever is up.
		for (let ifn in ["wan", "internet", "wwan", "lan"]) {
			let s = ctx.ubus.call(`network.interface.${ifn}`, "status");
			if (!s) continue;
			let isup = s.up ? true : false;
			let has_ip = (s["ipv4-address"] && length(s["ipv4-address"]) > 0);
			push(status.network, {
				interface: ifn,
				up: isup,
				proto: s.proto ?? "",
				online: (ifn == "lan") ? true : (isup && has_ip)
			});
		}
		return status;
	},

	// Faithful port of the real handler: tzoffset is the "+0000" STRING (not seconds),
	// and it returns localtime (from ubus system info) + timestamp for the date/time UI.
	get_timezone_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let info = ctx.ubus.call("system", "info") ?? {};
		return {
			localtime: info.localtime ?? time(),
			zonename: c.get("system", "@system[0]", "zonename") ?? "UTC",
			timezone: c.get("system", "@system[0]", "timezone") ?? "UTC",
			autotimezone_enabled: (c.get("glconfig", "general", "autotimezone") == "1"),
			tzoffset: ctx.popen("date +'%z' 2>/dev/null") ?? "+0000",
			timestamp: time()
		};
	},

	// Faithful port of set_timezone_config: writes system.@system[0].timezone/zonename,
	// disables auto-timezone, optionally sets the clock from `localtime` (epoch), and
	// reloads the system service. Returns the fresh config (matching the real handler).
	set_timezone_config: function(params, ctx) {
		let E = ctx.util.E;
		let zonename = params.zonename;
		let timezone = params.timezone;
		let localtime = params.localtime;

		if (zonename != "UTC" && timezone != "UTC" &&
		    (type(zonename) != "string" || type(timezone) != "string" || trim(timezone) == ""))
			return E.INVALID_PARAMS;

		let c = ctx.uci.cursor();
		if (zonename == "UTC" || timezone == "UTC")
			c.set("system", "@system[0]", "timezone", "UTC");
		else
			c.set("system", "@system[0]", "timezone", timezone);
		c.set("system", "@system[0]", "zonename", zonename);
		c.commit("system");

		// manual set disables auto-timezone; get_timezone_config reads glconfig.general.
		// (uci can't commit into a config file that doesn't exist yet -> create it first.)
		if (!fs.access("/etc/config/glconfig")) {
			let t = fs.open("/etc/config/glconfig", "w");
			if (t) t.close();
		}
		c.set("glconfig", "general", "service");
		c.set("glconfig", "general", "autotimezone", "0");
		c.commit("glconfig");
		ctx.popen("sync 2>/dev/null");

		if (localtime != null) {
			if (type(localtime) != "int" && type(localtime) != "double")
				return { err_code: -1, err_msg: "localtime format error" };
			ctx.popen(sprintf("date -s @%d >/dev/null 2>&1", int(localtime)));
			fs.writefile("/var/state/ntp-valid", "user set\n");
			fs.writefile("/var/state/dnsmasqsec", "user set\n");
			fs.writefile("/tmp/modem-ntp-valid", "user set\n");
			ctx.popen("/etc/init.d/chronyd restart >/dev/null 2>&1");
		}
		ctx.popen("/etc/init.d/system reload >/dev/null 2>&1");
		return this.get_timezone_config({}, ctx);
	},

	reboot: function(args, ctx) {
		ctx.popen("(sleep 1; reboot) >/dev/null 2>&1 &");
		return {};
	}
};
