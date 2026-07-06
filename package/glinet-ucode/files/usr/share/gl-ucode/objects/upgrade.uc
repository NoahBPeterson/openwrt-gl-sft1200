// gl-ucode object: upgrade  (port of decompiled-spec/rpc/upgrade.lua, 4.9.0)
// Firmware OTA + local flash. GL C helpers dropped/degraded: lfactory.* (model/
// country/firmware-url/max-size/reduce-check) -> ubus/procfs/uci; the
// /usr/bin/one_click_upgrade C helper -> a curl->sha256->sysupgrade pipeline
// script (/tmp/gl_ota.sh); resty.http -> curl via ctx.popen. DROPPED entirely:
// the 2b / partner-release-note / e5800 / cellular-modem code paths (no modem on
// SFT1200 -> the 6 cellular methods are stubs). PRESERVED: method + field names,
// the status-code enums, and the /tmp/firmware.img, /tmp/sysupgrade.meta,
// /tmp/upgrade_status file contract.
'use strict';

const fs = require('fs');

// ---- file / string helpers ------------------------------------------------
function readf(p)      { let d = fs.readfile(p); return d != null ? d : null; }
function readline(p)   { let d = fs.readfile(p); if (d == null) return null; return trim(split(d, "\n")[0]); }
function writef(p, v)  { fs.writefile(p, `${v}`); }
function fexists(p)    { return fs.access(p) ? true : false; }

// leading-integer of a value (Lua version_part_number / *:match("^(%d+)"))
function numlead(v) { let m = match(`${v ?? ""}`, /^([0-9]+)/); return m ? int(m[1]) : 0; }

// "a.b.c" -> [a,b,c] ints, or null (parse_version_string)
function parse_ver(s) {
	if (s == null) return null;
	let m = match(s, /([0-9]+)\.([0-9]+)\.([0-9]+)/);
	return m ? [ int(m[1]), int(m[2]), int(m[3]) ] : null;
}
// calculate_version(): -1 when incomplete
function calc_ver(v) { return v ? (v[0] * 10000 + v[1] * 100 + v[2]) : -1; }

// first three dot-separated parts of a version string (split_version_parts)
function split3(v) {
	if (type(v) != "string") return null;
	let m = match(v, /^([^.]+)\.([^.]+)\.([^.]+)/);
	return m ? [ m[1], m[2], m[3] ] : null;
}

function hexint(h) {
	let n = 0; h = lc(`${h ?? ""}`);
	for (let i = 0; i < length(h); i++) {
		let d = index("0123456789abcdef", substr(h, i, 1));
		if (d < 0) break;
		n = n * 16 + d;
	}
	return n;
}

// ---- system introspection (degrades: GL helpers -> procfs/ubus/uci) --------
function model(ctx) {
	let c = ctx.uci.cursor();
	let b = ctx.ubus.call("system", "board") ?? {};
	return readline("/proc/gl-hw-info/model")
	    ?? c.get("board_special", "hardware", "model")
	    ?? b.board_name
	    ?? "sft1200";
}

function country_code(ctx) {
	let c = ctx.uci.cursor();
	// DEGRADE: lfactory.get_country_code() absent -> wireless country / default US.
	let cc = c.get("wireless", "@wifi-device[0]", "country")
	      ?? c.get("glconfig", "general", "country");
	return (cc && cc != "") ? cc : "US";
}

// ---- shell wrappers -------------------------------------------------------
// returns the exit status (int) of cmd; 1 when popen yields nothing.
function run_status(ctx, cmd) {
	let o = ctx.popen(`${cmd} >/dev/null 2>&1; echo $?`);
	return o != null ? int(trim(o)) : 1;
}
function shell_out(ctx, cmd) { return ctx.popen(cmd) ?? ""; }

// ---- HTTP via curl (degrade: resty.http absent) ---------------------------
const SSL_HOST      = "fw.gl-inet.com";
const GRAY_URL      = "https://firmware-api.gl-inet.com";
const GRAY_CN_URL   = "https://firmware-api.gl-inet.cn";
const GRAY_CHECK    = "/api/firmware/check";
const GRAY_DOWNLOAD = "/api/firmware/download";

function http_get(ctx, url) {
	let body = ctx.popen(`curl -Ls --connect-timeout 5 -m 15 "${url}" 2>/dev/null`);
	return (body && body != "") ? body : null;
}
function http_post_json(ctx, url, obj) {
	writef("/tmp/upgrade_req.json", json(obj));
	let body = ctx.popen(`curl -Ls --connect-timeout 3 -m 10 -X POST ` +
		`-H 'Content-Type: application/json; charset=utf-8' ` +
		`--data @/tmp/upgrade_req.json "${url}" 2>/dev/null`);
	return (body && body != "") ? body : null;
}
function json_safe(s) {
	if (type(s) != "string" || s == "") return null;
	let r; try { r = json(s); } catch (e) { r = null; }
	return r;
}

// ---- constants (preserve the original enums / file paths) ------------------
const ERR_PARAMETER          = -1;
const MEMORY_ENOUGH          = 20008010;
const MEMORY_UNENOUGH        = 20008011;
const NEW_FIRMWARE_SHA256    = "/tmp/firmware_sha256";
const NEW_FIRMWARE_SIZE      = "/tmp/firmware_size";
const NEW_FIRMWARE_URL       = "/tmp/firmware_url";
const NEW_FIRMWARE_METHOD    = "/tmp/firmware_method";
const META_FILE              = "/tmp/sysupgrade.meta";
const PROCESS_MODEM          = "/tmp/process_modem";
const FW_METHOD_GRAY         = 0;
const FW_METHOD_RELEASE      = 1;
const FW_METHOD_LOCAL        = 2;

// ---- upgrade-param validation (is_valid_upgrade_params) --------------------
function validate_upgrade_params(p) {
	if (type(p.keep_config)  != "bool") return { err_code: ERR_PARAMETER, err_msg: "keep_config is invalid" };
	if (type(p.keep_package) != "bool") return { err_code: ERR_PARAMETER, err_msg: "keep_package is invalid" };
	if (p.url != null && !match(p.url, /^https?:\/\/[a-zA-Z0-9._?:\/+=&-]+$/))
		return { err_code: ERR_PARAMETER, err_msg: "url is invalid" };
	if (p.url != null && p.sha256 == null)
		return { err_code: ERR_PARAMETER, err_msg: "found url but sha256" };
	if (p.sha256 != null && (!match(p.sha256, /^[a-fA-F0-9]+$/) || length(p.sha256) != 64))
		return { err_code: ERR_PARAMETER, err_msg: "sha256 is invalid" };
	return null;
}

// available memory in bytes (get_available_space)
function available_space(ctx) {
	let info = ctx.ubus.call("system", "info") ?? {};
	let mem = info.memory ?? {};
	return int(mem.available ?? mem.free ?? 0);
}
// tmpfs size in MB (get_tmp_size)
function tmp_size_mb(ctx) {
	let o = ctx.popen("df -m /tmp 2>/dev/null | awk 'NR==2{print $2}'");
	return (o && trim(o) != "") ? int(trim(o)) : null;
}

// firmware partition max size in bytes from /proc/mtd (degrade: lfactory
// platform_get_firmware_max_size). 0 when undeterminable -> too-big check skipped.
function firmware_max_size() {
	let mtd = readf("/proc/mtd");
	if (mtd == null) return 0;
	for (let want in ["firmware", "rootfs", "ubi"]) {
		for (let line in split(mtd, "\n")) {
			if (index(line, `"${want}"`) >= 0) {
				let m = match(line, /:[ \t]+([0-9a-fA-F]+)/);
				if (m) return hexint(m[1]);
			}
		}
	}
	return 0;
}

// ============================ online version metadata =======================
function firmware_metadata(ctx, base_url, new_version) {
	let body = http_get(ctx, `${base_url}/metadata_${new_version}`);
	let md = json_safe(body);
	if (!md) return null;
	writef("/tmp/firmware_metadata", body);
	let compile_time = "", ftype = "", build_num;
	let ver = md.version;
	if (ver) {
		let dm = match(`${ver.date ?? ""}`, /([0-9]{4})([0-9]{2})([0-9]{2})([0-9]{2})([0-9]{2})([0-9]{2})/);
		if (dm) compile_time = `${dm[1]}-${dm[2]}-${dm[3]} ${dm[4]}:${dm[5]}:${dm[6]}`;
		ftype = (index(`${ver.firmware_type ?? ""}`, "release") >= 0) ? "release" : (ver.firmware_type ?? "");
		build_num = ver.build_num;
	}
	return {
		new_compile_time: compile_time,
		new_firmware_type: ftype,
		release_note: (type(md.release_note) == "string") ? md.release_note : "",
		release_note_cn: (type(md.release_note_cn) == "string") ? md.release_note_cn : "",
		build_num: build_num
	};
}

// device fingerprint POSTed to the firmware API (get_upload_data). null when the
// required /etc/version.* facts are absent (mainline degrade -> no online check).
function upload_data(ctx, upgrade_method) {
	let c = ctx.uci.cursor();
	let build_num    = readline("/etc/version.build") ?? "";
	let version_type = readline("/etc/version.type")  ?? "";
	let d = {
		model:        model(ctx),
		country_code: country_code(ctx),
		model_type:   readline("/etc/firmware_type") ?? "",
		version:      readline("/etc/glversion") ?? ""
	};
	if (build_num == "" || version_type == "" || d.model == "" || d.version == "")
		return null;
	d.timezone     = c.get("system", "@system[0]", "timezone") ?? "";
	d.language     = c.get("oui-httpd", "main", "lang") ?? "";
	d.build_number = `${d.model}_${d.version}_${version_type}-${build_num}`;
	if (upgrade_method != null) d.upgrade_method = upgrade_method;
	return d;
}

function check_resp_data(r) {
	if (!r) return false;
	for (let k in ["upgrade_available", "release_note_cn", "firmware_path",
	               "release_note", "new_version", "sha256", "size"])
		if (type(r[k]) != "string") return false;
	return true;
}

function report_gray_api(ctx, up, api) {
	let c = ctx.uci.cursor();
	let base = (up.country_code == "CN") ? GRAY_CN_URL : GRAY_URL;
	let url = c.get("upgrade", "general", "gray_url") ?? base;
	return http_post_json(ctx, url + api, up);
}

// gray (firmware-api POST) check. returns true + fills res on a hit.
function gray_release_version(ctx, res) {
	let c = ctx.uci.cursor();
	let up = upload_data(ctx);
	if (!up || c.get("upgrade", "general", "gray_disable") == "1") return false;
	let ignore_version = c.get("upgrade", "general", "ignore_gray_version");
	let gray_prompt = c.get("upgrade", "general", "gray_prompt") ?? "1";
	let body = report_gray_api(ctx, up, GRAY_CHECK);
	let r = json_safe(body);
	if (!check_resp_data(r) || !r.upgrade_available) return false;

	if (gray_prompt == "0" && r.new_version != ignore_version) {
		res.prompt = true; c.set("upgrade", "general", "gray_prompt", "1");
	} else if (gray_prompt == "1") {
		c.set("upgrade", "general", "ignore_gray_version", r.new_version);
	}
	res.prompt = (gray_prompt == "1");
	res.new_firmware_type = "release";
	res.release_note      = r.release_note;
	res.release_note_cn   = r.release_note_cn;
	res.version_new       = r.new_version;
	res.size              = int(r.size) || 0;
	writef(NEW_FIRMWARE_SHA256, r.sha256);
	writef(NEW_FIRMWARE_SIZE,   r.size);
	writef(NEW_FIRMWARE_URL,    r.firmware_path);
	writef(NEW_FIRMWARE_METHOD, FW_METHOD_GRAY);
	c.set("upgrade", "general", "method", "gray");
	c.commit("upgrade");
	return true;
}

// release (list-sha256.txt) check. returns true + fills res on a newer build.
function release_firmware_version(ctx, current_version, res) {
	if (!current_version || current_version == "") return false;
	let c = ctx.uci.cursor();
	let ignore_version = c.get("upgrade", "general", "ignore_version");
	let prompt = c.get("upgrade", "general", "prompt") ?? "1";

	// DEGRADE: lfactory.platform_get_frimware_url() -> hardcoded fw.gl-inet.com
	// path built from model + firmware type.
	let ftype = readline("/etc/firmware_type") ?? "release";
	let base_url = `https://${SSL_HOST}/firmware/${model(ctx)}/${ftype}`;

	let list = http_get(ctx, `${base_url}/list-sha256.txt`);
	if (!list || match(list, /html/)) return false;
	writef("/tmp/firmware_list", list);
	let cur_build = int(readline("/etc/version.build") ?? "0");
	let cur = calc_ver(parse_ver(current_version));

	for (let line in split(list, "\n")) {
		let cols = split(trim(line), "\t");
		if (length(cols) < 4 || length(cols) > 5) continue;
		let new_version = cols[0], fw_name = cols[1], sha256 = cols[2], size = cols[3];
		let supported = (length(cols) == 5) ? cols[4] : null;
		if (supported && calc_ver(parse_ver(supported)) > cur) continue;
		let md = firmware_metadata(ctx, base_url, new_version);
		if (!md) continue;
		let nv = calc_ver(parse_ver(new_version));
		let newer = (cur < nv) || (nv == cur && md.build_num && cur_build < int(md.build_num));
		if (newer) {
			if (prompt == "0" && new_version != ignore_version) {
				res.prompt = true; c.set("upgrade", "general", "prompt", "1");
			} else if (prompt == "1") {
				c.set("upgrade", "general", "ignore_version", new_version);
			}
			res.prompt            = (prompt == "1");
			res.version_new       = new_version;
			res.size              = int(size) || 0;
			res.new_compile_time  = md.new_compile_time;
			res.new_firmware_type = (index(md.new_firmware_type, "release") >= 0) ? "release" : md.new_firmware_type;
			res.release_note      = md.release_note;
			res.release_note_cn   = md.release_note_cn;
			writef(NEW_FIRMWARE_SHA256, sha256);
			writef(NEW_FIRMWARE_SIZE,   size);
			writef(NEW_FIRMWARE_URL,    `${base_url}/${fw_name}`);
			writef(NEW_FIRMWARE_METHOD, FW_METHOD_RELEASE);
			c.set("upgrade", "general", "method", "stable");
			c.commit("upgrade");
			return true;
		} else if (cur > nv) {
			return false;
		}
	}
	return false;
}

// ============================ local firmware version check ==================
function metadata_release(md)   { return (md && md.version && type(md.version.release) == "string") ? md.version.release : ""; }
function metadata_supported(md) { return (md && md.upgrade_control && type(md.upgrade_control.supported_version) == "string") ? md.upgrade_control.supported_version : ""; }
function local_firmware_type(md) {
	if (!md || !md.version) return "";
	let t = md.version.firmware_type ?? "";
	return (index(t, "release") >= 0) ? "release" : t;
}
function set_upgrade_control_info(res, md) {
	if (!md || type(md.upgrade_control) != "object") return;
	let ctl = md.upgrade_control;
	if (ctl.not_keep_config != null) res.not_keep_config = ctl.not_keep_config;
	if (type(ctl.not_keep_config_part) == "array" && length(ctl.not_keep_config_part) > 0)
		res.not_keep_config_part = ctl.not_keep_config_part;
}

// port of apply_local_version_check -> [status, ret_flag]
function apply_local_version_check(res, md, status, ret_flag) {
	let new_ver = metadata_release(md);
	if (ret_flag || new_ver == "") {
		if (!ret_flag) { res.version = "unknown"; return [3, true]; }
		return [status, ret_flag];
	}
	let cur_ver = readline("/etc/glversion") ?? "";
	// DEGRADE: /etc/gliotversion (modem IoT split) absent -> router-only compare.
	let cv = split3(cur_ver), nv = split3(new_ver);
	if (!cv || !nv) {
		status = 1; ret_flag = true;
	} else {
		let low = metadata_supported(md);
		if (numlead(low) > 0) {
			let lv = split3(low);
			if (lv) {
				if (numlead(cv[0]) < numlead(lv[0])) { status = 7; ret_flag = true; }
				else if (numlead(cv[0]) == numlead(lv[0])) {
					if (numlead(cv[1]) < numlead(lv[1])) { status = 7; ret_flag = true; }
					else if (numlead(cv[1]) == numlead(lv[1]) && numlead(cv[2]) < numlead(lv[2])) { status = 7; ret_flag = true; }
				}
			}
		}
		if (!ret_flag) {
			if (numlead(nv[0]) >= numlead(cv[0])) {
				if (numlead(nv[0]) > numlead(cv[0])) { status = 2; ret_flag = true; }
				else if (numlead(nv[1]) >= numlead(cv[1])) {
					if (numlead(nv[1]) == numlead(cv[1]) && numlead(nv[2]) < numlead(cv[2])) { status = 1; ret_flag = true; }
					// equal version -> leave status as-is (no upgrade/downgrade)
				} else { status = 1; ret_flag = true; }
			} else { status = 1; ret_flag = true; }
		}
		// DEGRADE: lfactory.reduce_version_upgrade_check() (downgrade gate) dropped.
	}
	res.version = new_ver;
	writef(PROCESS_MODEM, res.version);
	return [status, ret_flag];
}

// removes the not_keep_config_part paths named in the meta (handle_upgrade_control)
function handle_upgrade_control(meta) {
	let conf = json_safe(readf(meta));
	let part = conf && conf.upgrade_control ? conf.upgrade_control.not_keep_config_part : null;
	if (type(part) != "array") return;
	for (let p in part)
		if (p && p.path) try { fs.unlink(p.path); } catch (e) {}
}

// cellular upgrade is unsupported on SFT1200 (no modem).
const CELLULAR_UNSUPPORTED = { err_code: -1, err_msg: "cellular upgrade not supported on this device" };

export default {
	// ---------------------- ONLINE OTA (degraded pipeline) ----------------
	upgrade_online: function(args, ctx) {
		let bad = validate_upgrade_params(args);
		if (bad) return bad;

		let firmware_url = args.url;
		let sha256 = args.sha256;
		let id = args.id;
		let firmware_size = (args.size != null) ? int(args.size) : null;
		let keep_config = args.keep_config, keep_package = args.keep_package;
		let need_reboot_flag = true;

		// resolve url/sha/size from the check_firmware_online staging files
		if (!firmware_url) {
			firmware_url  = readline(NEW_FIRMWARE_URL)  ?? "no url";
			sha256        = readline(NEW_FIRMWARE_SHA256) ?? "0";
			firmware_size = int(readline(NEW_FIRMWARE_SIZE) ?? "0");
		}
		let need_size = firmware_size;

		// tmpfs capacity guard
		let tmb = tmp_size_mb(ctx);
		if (tmb != null && need_size) {
			if (need_size >= tmb * 1024 * 1024) {
				if (id) writef("/tmp/upgrade_status", MEMORY_UNENOUGH);
				return { need_reboot_flag: need_reboot_flag };
			}
		}
		// RAM guard (retry after dropping caches)
		let cur = available_space(ctx);
		if (need_size && cur && need_size >= cur) {
			ctx.popen("echo 3 > /proc/sys/vm/drop_caches");
			if (need_size >= available_space(ctx)) {
				if (id) writef("/tmp/upgrade_status", MEMORY_UNENOUGH);
				return { need_reboot_flag: need_reboot_flag };
			}
		}
		if (id) writef("/tmp/upgrade_status", MEMORY_ENOUGH);

		try { fs.unlink("/tmp/firmware.img"); } catch (e) {}

		// DEGRADE: /usr/bin/one_click_upgrade (closed C helper) reimplemented as a
		// curl -> sha256 -> sysupgrade -T -> sysupgrade pipeline. Status codes match
		// get_online_upgrade_status()'s enum (1 dl,2 dl-fail,3 dl-ok,4 hash-ok,
		// 5 hash/verify-fail,6 platform-ok).
		let n_flag = keep_config ? "" : "-n";
		let k_flag = keep_package ? "-k" : "";
		let script =
			"#!/bin/sh\n" +
			"echo 1 > /tmp/upgrade_status\n" +
			`curl -Ls --connect-timeout 5 -o /tmp/firmware.img "${firmware_url}" || { echo 2 > /tmp/upgrade_status; exit 1; }\n` +
			"echo 3 > /tmp/upgrade_status\n" +
			`GOT=$(sha256sum /tmp/firmware.img | awk '{print $1}')\n` +
			`[ "$GOT" = "${sha256}" ] || { echo 5 > /tmp/upgrade_status; exit 1; }\n` +
			"echo 4 > /tmp/upgrade_status\n" +
			"sysupgrade -T /tmp/firmware.img >/dev/null 2>&1 || { echo 5 > /tmp/upgrade_status; exit 1; }\n" +
			"echo 6 > /tmp/upgrade_status\n" +
			`sysupgrade ${k_flag} ${n_flag} /tmp/firmware.img >/dev/null 2>&1\n`;
		writef("/tmp/gl_ota.sh", script);
		writef("/tmp/upgrade_status", "1");
		ctx.popen("(sleep 1; sh /tmp/gl_ota.sh) >/dev/null 2>&1 &");
		return {};
	},

	upgrade_online_cancel: function(args, ctx) {
		let result = {};
		if (fexists("/tmp/download_ok"))
			return { err_code: -3, err_msg: "Upgrade process is already started, cannot cancel" };
		// DEGRADE: kill our curl/sh pipeline rather than the one_click_upgrade helper.
		let pids = ctx.popen("ps | grep -E 'gl_ota.sh|curl -Ls --connect-timeout 5' | grep -v grep | awk '{print $1}'");
		for (let pid in split(pids ?? "", "\n")) {
			pid = trim(pid);
			if (match(pid, /^[0-9]+$/)) ctx.popen(`kill ${pid} 2>/dev/null`);
		}
		try { fs.unlink("/tmp/one_click_upgrade_pid"); } catch (e) {}
		if (fexists("/tmp/firmware.img")) try { fs.unlink("/tmp/firmware.img"); } catch (e) {}
		try { fs.unlink("/var/upgrade_status"); } catch (e) {}
		try { fs.unlink("/tmp/upgrade_status"); } catch (e) {}
		return result;
	},

	report_upgrade_method: function(args, ctx) {
		let method = readline(NEW_FIRMWARE_METHOD);
		if (method == null || method == "") return {};
		let up = upload_data(ctx, int(method));
		if (up) {
			let fw = readline(NEW_FIRMWARE_URL) ?? "";
			let m = match(fw, /\/([^\/]+\/[^\/]+)$/);
			up.file_name = m ? m[1] : "";
			report_gray_api(ctx, up, GRAY_DOWNLOAD);
		}
		return {};
	},

	check_firmware_online: function(args, ctx) {
		// GL's version files (/etc/glversion, version.date, version.type) are empty on
		// mainline, so fall back to OpenWrt sources — otherwise the Firmware page shows
		// blank version/type/compile-time. Keep the version consistent with system.get_info.
		let current_version = readline("/etc/glversion");
		if (!current_version || current_version == "") current_version = "4.8.3";
		let current_type = readline("/etc/version.type");
		if (!current_type || current_type == "") current_type = "release";
		let current_compile_time = readline("/etc/version.date");
		if (!current_compile_time || current_compile_time == "")
			current_compile_time = ctx.popen("date -r /etc/openwrt_release '+%Y-%m-%d %H:%M:%S' 2>/dev/null")
				?? readline("/etc/openwrt_version") ?? "";
		let res = {
			current_version: current_version,
			current_compile_time: current_compile_time,
			current_type: (index(current_type, "release") >= 0) ? "release" : current_type
		};
		// DEGRADE: 2b model-match path dropped. On no-network the curl helpers
		// return null and we fall through returning only the current version.
		if (!gray_release_version(ctx, res))
			release_firmware_version(ctx, current_version, res);
		if (res.version_new) writef(PROCESS_MODEM, `${res.version_new}\n`);
		return res;
	},

	get_online_upgrade_status: function(args, ctx) {
		let status_msgs = {
			"1": "downloading", "2": "download failed", "3": "download ok",
			"4": "hash verify ok", "5": "hash verify failed",
			"6": "hash verify ok", "9": "firmware signature verify failed"
		};
		let sline = readline("/tmp/upgrade_status");
		if (sline == null || sline == "") return {};
		let status = int(sline);
		let res = { status: status, status_msg: status_msgs[status] };

		if (status == 1) {
			res.percent = 0;
			let total = int(readline("/tmp/firmware_size") ?? "0");
			if (!total) {
				let list = readf("/tmp/firmware_list");
				if (!list) return res;
				let m = match(list, /\S+\s+\S+\s+\S+\s+([0-9]+)/);
				if (!m) { res.status = 2; res.status_msg = status_msgs[2]; return res; }
				total = int(m[1]);
				if (!total || total <= 0) { res.status = 2; res.status_msg = status_msgs[2]; return res; }
				writef("/tmp/firmware_size", `${total}\n`);
			}
			let st = fs.stat("/tmp/firmware.img");
			if (!st) return res;
			if (total <= 0) { res.status = 2; res.status_msg = status_msgs[2]; return res; }
			let percent = st.size / total;
			res.percent = percent - (percent % 0.01);
			return res;
		}

		try { fs.unlink("/tmp/firmware_size"); } catch (e) {}
		if (status == 6) {
			// platform_check_image is validation-only (no flash) — safe in a getter.
			let out = shell_out(ctx, ". /lib/functions.sh; include /lib/upgrade; platform_check_image /tmp/firmware.img 2>/dev/null");
			if (length(out) > 0) {
				writef("/tmp/upgrade_status", "5\n");
				res.status_msg = "platform verify fail";
			}
		}
		return res;
	},

	// ---------------------- LOCAL FLASH (pure mainline) -------------------
	check_firmware_local: function(args, ctx) {
		let res = {};
		let status = 0, ret_flag = false, firmware_signature = true;
		let firmware_path = "/tmp/firmware.img";

		// 1) image sanity: sysupgrade -T. status 6 = incompatible.
		if (run_status(ctx, "sysupgrade -T /tmp/firmware.img") != 0) { status = 6; ret_flag = true; }

		// 2) sha256 of the uploaded image
		res.sha256 = trim(shell_out(ctx, `sha256sum ${firmware_path} | awk '{print $1}'`));

		// 3) detached-signature verify (fwtool -t -s -> usign -V)
		ctx.popen(`fwtool -t -s /tmp/firmware.sig ${firmware_path} >/dev/null 2>&1`);
		firmware_signature = run_status(ctx,
			`usign -V -p /etc/key-build.pub -x /tmp/firmware.sig -m ${firmware_path} -q`) == 0;

		// 4) version comparison from the staged metadata
		let md = json_safe(readf(META_FILE));
		let r = apply_local_version_check(res, md, status, ret_flag);
		status = r[0]; ret_flag = r[1];

		// 5) size / platform checks
		let st = fs.stat(firmware_path);
		if (st) {
			let max_size = firmware_max_size();   // DEGRADE: 0 when /proc/mtd has no fw partition
			if (max_size > 0 && max_size < st.size) { status = 4; ret_flag = true; }        // too big
			if (st.size < 1048576 && !ret_flag)     { status = 10; ret_flag = true; }        // too small
			if (run_status(ctx, ". /lib/functions.sh; include /lib/upgrade; platform_check_image " + firmware_path) != 0) {
				status = 8; ret_flag = true;                                                 // platform fail
				try { fs.unlink("/tmp/firmware.img"); } catch (e) {}
			}
		} else {
			status = 5; ret_flag = true;                                                     // missing
		}

		set_upgrade_control_info(res, md);
		if (!firmware_signature && !ret_flag) status = 9;                                     // sig fail
		res.status = status;

		if (type(md) != "object") return res;
		res.firmware_type = local_firmware_type(md);
		// DEGRADE: 2b customer path dropped, lfactory.get_country_code -> wireless/US.
		if ((readline("/etc/firmware_type") ?? "") == "2b") return res;
		res.country_code    = country_code(ctx);
		res.release_note    = (type(md.release_note) == "string") ? md.release_note : "";
		res.release_note_cn = (type(md.release_note_cn) == "string") ? md.release_note_cn : "";
		return res;
	},

	upgrade_local: function(args, ctx) {
		if (!fexists("/tmp/firmware.img"))
			return { err_code: -2, err_msg: "firmware.img does not exist" };

		handle_upgrade_control(META_FILE);
		let keep_config = args.keep_config, keep_package = args.keep_package;

		let c = ctx.uci.cursor();
		c.set("upgrade", "general", "prompt", "1");
		c.set("upgrade", "general", "gray_prompt", "1");
		c.commit("upgrade");

		// best-effort log/pstore wipe (mainline: mtd 'log' partition or pstore)
		ctx.popen("[ -d /sys/fs/pstore ] && rm -f /sys/fs/pstore/* 2>/dev/null");
		let mtd = readf("/proc/mtd") ?? "";
		if (index(mtd, '"log"') >= 0)
			ctx.popen("/sbin/mtd erase log >/dev/null 2>&1");

		writef(NEW_FIRMWARE_METHOD, FW_METHOD_LOCAL);

		// DEGRADE: e5800/cellular_manager branches dropped. Straight mainline
		// sysupgrade, backgrounded so the RPC can return before the flash.
		let n_flag = keep_config ? "" : "-n";
		let k_flag = keep_package ? "-k" : "";
		let cmd = `killall dropbear 2>/dev/null; /etc/init.d/nginx stop 2>/dev/null; ` +
			`/etc/init.d/uhttpd stop 2>/dev/null; /sbin/sysupgrade ${k_flag} ${n_flag} /tmp/firmware.img`;
		ctx.popen(`(sleep 2; ${cmd}) >/dev/null 2>&1 &`);
		return {};
	},

	// ---------------------- config ---------------------------------------
	set_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let method = (c.get("upgrade", "general", "method") == "gray") ? "gray_" : "";
		if (args.prompt != null)
			c.set("upgrade", "general", `${method}prompt`, args.prompt ? "1" : "0");
		if (type(args.upgrade_enable) == "bool")
			c.set("upgrade", "general", "upgrade_enable", args.upgrade_enable ? "1" : "0");
		c.commit("upgrade");
		return {};
	},

	get_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let method = (c.get("upgrade", "general", "method") == "gray") ? "gray" : "";
		return {
			prompt: c.get("upgrade", "general", `${method}prompt`) == "1",
			upgrade_enable: (c.get("upgrade", "general", "upgrade_enable") ?? "1") == "1"
		};
	},

	// ---------------------- cellular (STUB: no modem on SFT1200) ----------
	// DEGRADE: the LTE/5G modem stack (gl_modem, upgrade_cellular, RM520/EM160/
	// EG120K) does not exist on this device. Field names preserved.
	check_cellular_online:        function(args, ctx) { return CELLULAR_UNSUPPORTED; },
	upgrade_cellular_online:      function(args, ctx) { return CELLULAR_UNSUPPORTED; },
	check_cellular_local:         function(args, ctx) { return CELLULAR_UNSUPPORTED; },
	upgrade_cellular_local:       function(args, ctx) { return CELLULAR_UNSUPPORTED; },
	get_cellular_upgrade_status:  function(args, ctx) { return { status: 0 }; },
	reset_cellular_upgrade_status: function(args, ctx) { return {}; }
};
