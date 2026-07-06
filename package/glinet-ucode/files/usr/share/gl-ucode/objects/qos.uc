// gl-ucode object: qos  (port of decompiled-spec/rpc/qos.lua)
// Per-client bandwidth limits / priority. The original was a front-end to GL's
// closed `gl_eqos` engine (which leaned on Siflower/MediaTek hw-offload) writing
// GL's bespoke uci `qos` schema. There is no mainline consumer of that schema, so
// this port keeps GL's `qos` uci as its OWN config store (all get/set round-trip
// faithfully) and replaces the ENFORCEMENT.
// DEGRADE: `gl_eqos` (start / reload_device_group / del) is absent. Per-client
// rate limits are enforced with tc HTB + flower MAC filters (see apply_speed_limits):
// download = egress on br-lan matched by destination MAC, upload = ingress on
// br-lan mirrored to an IFB device matched by source MAC. If tc/IFB are unavailable
// the rules are store-only. The global/priority/work_mode/packet_priority knobs
// (priority queues, packet marking, DPI work modes) have no mainline tc equivalent
// and are STORE-ONLY (persisted so the app round-trips; not enforced).
'use strict';

const LAN = "br-lan";
const IFB = "ifb-qos";

// upload/download queue values are treated as kbit/s for tc (the app's native unit).
function valid_mac(mac) {
	return (type(mac) == "string" && length(mac) == 17 && match(mac, /^[0-9a-fA-F:]+$/) != null);
}

function have_tc(ctx) {
	let v = ctx.popen("command -v tc 2>/dev/null");
	return (type(v) == "string" && length(v) > 0);
}

// DEGRADE: replaces `gl_eqos`. Rebuilds the whole tc shaping tree from the current
// uci `qos` `queue` sections (idempotent). Honours qos.general.enable. Best-effort:
// any missing tool / device leaves the rules store-only.
function apply_speed_limits(ctx) {
	// tear down prior shaping first (idempotent, safe if nothing exists)
	ctx.popen(`tc qdisc del dev ${LAN} root >/dev/null 2>&1`);
	ctx.popen(`tc qdisc del dev ${LAN} ingress >/dev/null 2>&1`);
	ctx.popen(`tc qdisc del dev ${IFB} root >/dev/null 2>&1`);
	ctx.popen(`ip link del ${IFB} >/dev/null 2>&1`);

	if (!have_tc(ctx)) return false;

	let c = ctx.uci.cursor();
	if (c.get("qos", "general", "enable") != "1") return true;   // disabled -> torn down

	let queues = [];
	c.foreach("qos", "queue", function(s) {
		let mac = s.mac;
		if (!valid_mac(mac)) return;
		let up = int(s.upload ?? 0);
		let dn = int(s.download ?? 0);
		if ((type(up) != "int") || up < 0) up = 0;
		if ((type(dn) != "int") || dn < 0) dn = 0;
		if (up <= 0 && dn <= 0) return;
		push(queues, { mac: lc(mac), up: up, dn: dn });
	});
	if (!length(queues)) return true;

	let any_up = false;
	for (let q in queues) if (q.up > 0) any_up = true;

	// download: shape egress on br-lan, classify by destination MAC
	ctx.popen(`tc qdisc add dev ${LAN} root handle 1: htb default 9999 >/dev/null 2>&1`);

	// upload: mirror br-lan ingress onto an IFB device, shape there by source MAC
	if (any_up) {
		ctx.popen(`ip link add ${IFB} type ifb >/dev/null 2>&1`);
		ctx.popen(`ip link set ${IFB} up >/dev/null 2>&1`);
		ctx.popen(`tc qdisc add dev ${IFB} root handle 1: htb default 9999 >/dev/null 2>&1`);
		ctx.popen(`tc qdisc add dev ${LAN} handle ffff: ingress >/dev/null 2>&1`);
		ctx.popen(`tc filter add dev ${LAN} parent ffff: protocol all u32 match u32 0 0 action mirred egress redirect dev ${IFB} >/dev/null 2>&1`);
	}

	let cid = 10;
	for (let q in queues) {
		if (q.dn > 0) {
			ctx.popen(`tc class add dev ${LAN} parent 1: classid 1:${cid} htb rate ${q.dn}kbit >/dev/null 2>&1`);
			ctx.popen(`tc filter add dev ${LAN} parent 1: protocol all flower dst_mac ${q.mac} action ok flowid 1:${cid} >/dev/null 2>&1`);
		}
		if (q.up > 0 && any_up) {
			ctx.popen(`tc class add dev ${IFB} parent 1: classid 1:${cid} htb rate ${q.up}kbit >/dev/null 2>&1`);
			ctx.popen(`tc filter add dev ${IFB} parent 1: protocol all flower src_mac ${q.mac} action ok flowid 1:${cid} >/dev/null 2>&1`);
		}
		cid++;
	}
	return true;
}

// get_client_list can fall back to live hosts (DHCP leases + ip neigh) like clients.uc
function leases(ctx) {
	let out = {}, raw = ctx.fs.readfile("/tmp/dhcp.leases");
	if (!raw) return out;
	for (let line in split(trim(raw), "\n")) {
		let f = split(line, " ");            // <expiry> <mac> <ip> <name> <id>
		if (length(f) >= 4) out[lc(f[1])] = { ip: f[2], name: (f[3] != "*") ? f[3] : "" };
	}
	return out;
}

export default {
	set_channel_bandwidth_ratio: function(args, ctx) {
		let c = ctx.uci.cursor();
		let sync_ratio = args.sync_ratio ? 1 : 0;
		c.set("qos", "general", "sync_ratio", sync_ratio);
		if (type(args.upload) == "array") {
			for (let item in args.upload) {
				let priority = item.priority;
				if (priority != null) {
					c.foreach("qos", "priority_rule", function(s) {
						let name = s[".name"];
						let priority_ori = c.get("qos", name, "priority");
						let direction = c.get("qos", name, "direction");
						if (`${priority}` == `${priority_ori}` && (sync_ratio == 1 || direction == "tx")) {
							c.set("qos", name, "min", item.min);
							c.set("qos", name, "max", item.max);
						}
					});
				}
			}
		}
		if (sync_ratio == 0 && type(args.download) == "array") {
			for (let item in args.download) {
				let priority = item.priority;
				if (priority != null) {
					c.foreach("qos", "priority_rule", function(s) {
						let name = s[".name"];
						let priority_ori = c.get("qos", name, "priority");
						let direction = c.get("qos", name, "direction");
						if (`${priority}` == `${priority_ori}` && direction == "rx") {
							c.set("qos", name, "min", item.min);
							c.set("qos", name, "max", item.max);
						}
					});
				}
			}
		}
		c.commit("qos");
		ctx.popen("sync");
		// DEGRADE: gl_eqos start dropped; priority-queue ratios are store-only (no mainline equivalent).
		return {};
	},

	get_channel_bandwidth_ratio: function(args, ctx) {
		let res = {};
		let upload = [];
		let download = [];
		let c = ctx.uci.cursor();
		res.sync_ratio = (c.get("qos", "general", "sync_ratio") == "1");
		c.foreach("qos", "priority_rule", function(s) {
			let name = s[".name"];
			let min = c.get("qos", name, "min");
			let max = c.get("qos", name, "max");
			let priority = c.get("qos", name, "priority");
			let direction = c.get("qos", name, "direction");
			if (direction == "tx")
				push(upload, { min: min, max: max, priority: priority });
			else
				push(download, { min: min, max: max, priority: priority });
		});
		res.upload = upload;
		res.download = download;
		return res;
	},

	add_device_group: function(args, ctx) {
		let c = ctx.uci.cursor();
		let group_id = args.group_id;
		if (group_id == null) {
			group_id = int(c.get("qos", "general", "group_number") ?? 0) + 1;
			c.set("qos", "general", "group_number", group_id);
		}
		if (args.group_name != null) {
			let gid = `${group_id}`;
			let new_group = c.add("qos", "group");
			c.rename("qos", new_group, gid);
			c.set("qos", gid, "name", args.group_name);
			c.set("qos", gid, "priority", args.priority);
			c.set("qos", gid, "mac", args.clients);
		}
		c.commit("qos");
		ctx.popen("sync");
		// DEGRADE: gl_eqos reload_device_group dropped; groups/priority are store-only.
		return {};
	},

	delete_device_group: function(args, ctx) {
		let group_id = args.group_id;
		if (group_id != null) {
			let c = ctx.uci.cursor();
			c.delete("qos", `${group_id}`);
			c.commit("qos");
			ctx.popen("sync");
			// DEGRADE: gl_eqos reload_device_group dropped.
			return {};
		}
		return { err_code: -5, err_msg: "parameter found" };
	},

	modify_device_group: function(args, ctx) {
		let group_id = args.group_id;
		if (group_id != null) {
			let c = ctx.uci.cursor();
			c.delete("qos", `${group_id}`);
			c.commit("qos");
			this.add_device_group(args, ctx);
			return {};
		}
		return { err_code: -5, err_msg: "parameter found" };
	},

	get_device_group: function(args, ctx) {
		let res = {};
		let group = [];
		let c = ctx.uci.cursor();
		c.foreach("qos", "group", function(s) {
			let group_id = s[".name"];
			group[length(group)] = {
				group_id: group_id,
				clients: c.get("qos", group_id, "mac"),
				name: c.get("qos", group_id, "name"),
				priority: c.get("qos", group_id, "priority")
			};
		});
		res.group = group;
		res.default_priority = c.get("qos", "general", "default_priority");
		return res;
	},

	set_default_priority: function(args, ctx) {
		let c = ctx.uci.cursor();
		c.set("qos", "general", "default_priority", args.priority ?? 1);
		c.commit("qos");
		ctx.popen("sync");
		// DEGRADE: gl_eqos start dropped; default priority is store-only.
		return {};
	},

	get_client_list: function(args, ctx) {
		let c = ctx.uci.cursor();
		let clients = [];
		let seen = {};
		c.foreach("qos", "queue", function(s) {
			if (!s.mac) return;
			seen[lc(s.mac)] = true;
			push(clients, {
				mac: s.mac,
				name: s.name,
				upload: int(s.upload ?? 0),
				download: int(s.download ?? 0)
			});
		});
		// DEGRADE: gl_eqos didn't source live hosts; enrich with DHCP leases so the
		// app can pick a device to limit even before any rule exists (name only).
		let lz = leases(ctx);
		for (let mac in lz) {
			if (seen[mac]) continue;
			push(clients, { mac: mac, name: lz[mac].name, upload: 0, download: 0 });
		}
		return { clients: clients };
	},

	set_speed_limit_rule: function(args, ctx) {
		let mac = args.mac;
		let upload = args.upload;
		let download = args.download;
		let c = ctx.uci.cursor();

		// DEGRADE: MTK mtkhnat / ecm hw-NAT branches dropped (MediaTek/QCA-only). On the
		// Siflower device the offload guard maps to fw4 flow_offloading; if hw offload is
		// on, offloaded flows bypass the shaper, so refuse like the original.
		let hnat_enable = (c.get("firewall", "@defaults[0]", "flow_offloading") == "1" ||
			c.get("firewall", "@defaults[0]", "flow_offloading_hw") == "1");
		if (hnat_enable)
			return ctx.util.E.INVALID_PARAMS;

		if (valid_mac(mac) && type(upload) == "int" && type(download) == "int") {
			if (upload != 0 || download != 0) {
				let sec_name;
				c.foreach("qos", "queue", function(s) {
					if (mac == s.mac) {
						sec_name = s[".name"];
						return false;
					}
				});
				if (!sec_name) {
					sec_name = replace(mac, ":", "");
					let new_sec = c.add("qos", "queue");
					c.rename("qos", new_sec, sec_name);
					c.set("qos", sec_name, "mac", mac);
				}
				c.set("qos", sec_name, "download", download);
				c.set("qos", sec_name, "upload", upload);
				c.commit("qos");
				ctx.popen("sync");
				apply_speed_limits(ctx);   // DEGRADE: replaces `gl_eqos reload`
				return {};
			}
			return this.remove_speed_limit_rule(args, ctx);
		}
		return ctx.util.E.INVALID_PARAMS;
	},

	remove_speed_limit_rule: function(args, ctx) {
		let mac = args.mac;
		if (!valid_mac(mac))
			return ctx.util.E.INVALID_PARAMS;
		let c = ctx.uci.cursor();
		// find the section by mac (sections are named with colons stripped); fall
		// back to the raw/stripped name to mirror the original delete.
		let sec_name;
		c.foreach("qos", "queue", function(s) {
			if (mac == s.mac) {
				sec_name = s[".name"];
				return false;
			}
		});
		c.delete("qos", sec_name ?? replace(mac, ":", ""));
		c.commit("qos");
		ctx.popen("sync");
		apply_speed_limits(ctx);   // DEGRADE: replaces `gl_eqos del <mac>`
		return {};
	},

	set_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		c.set("qos", "general", "enable", args.enable ? 1 : 0);
		c.set("qos", "general", "qos_mode", args.mode ?? 0);
		c.commit("qos");
		ctx.popen("sync");
		apply_speed_limits(ctx);   // DEGRADE: replaces `gl_eqos start`; (re)applies/tears down tc
		return {};
	},

	get_config: function(args, ctx) {
		let res = {};
		let c = ctx.uci.cursor();
		res.enable = (c.get("qos", "general", "enable") == "1");
		res.mode = c.get("qos", "general", "qos_mode") ?? "0";
		return res;
	},

	get_bandwidth_config: function(args, ctx) {
		let res = {};
		let band_cfg = [];
		let c = ctx.uci.cursor();
		c.foreach("qos", "interface", function(s) {
			let name = s[".name"];
			band_cfg[length(band_cfg)] = {
				interface: name,
				enable: (c.get("qos", name, "enable") == "1"),
				upload: c.get("qos", name, "upload"),
				download: c.get("qos", name, "download")
			};
		});
		res.band_cfg = band_cfg;
		return res;
	},

	set_bandwidth_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		if (type(args.band_cfg) == "array") {
			for (let item in args.band_cfg) {
				let interface = item.interface;
				if (interface != null) {
					let flag = 0;
					c.foreach("qos", "interface", function(s) {
						if (interface == s[".name"]) flag = 1;
					});
					if (flag != 1) {
						let new_interface = c.add("qos", "interface");
						c.rename("qos", new_interface, interface);
					}
					c.set("qos", interface, "enable", item.enable ? 1 : 0);
					c.set("qos", interface, "upload", item.upload);
					c.set("qos", interface, "download", item.download);
				}
			}
		}
		c.commit("qos");
		ctx.popen("sync");
		// DEGRADE: gl_eqos start dropped; per-interface bandwidth caps are store-only
		// (this was a WAN-side shaper in gl_eqos with no faithful br-lan tc equivalent).
		return {};
	},

	set_work_mode: function(args, ctx) {
		let c = ctx.uci.cursor();
		c.set("qos", "general", "work_mode", args.work_mode ?? 0);
		c.commit("qos");
		ctx.popen("sync");
		// DEGRADE: gl_eqos start dropped; work_mode (DPI/offload engine mode) is store-only.
		return {};
	},

	set_packet_priority: function(args, ctx) {
		let c = ctx.uci.cursor();
		c.set("qos", "general", "packet_priority", args.packet_priority ? 1 : 0);
		c.commit("qos");
		ctx.popen("sync");
		// DEGRADE: gl_eqos start dropped; packet_priority (DSCP marking) is store-only.
		return {};
	}
};
