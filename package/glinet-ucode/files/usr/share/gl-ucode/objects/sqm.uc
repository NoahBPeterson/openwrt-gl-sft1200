// gl-ucode object: sqm  (port of decompiled-spec/rpc/sqm.lua)
// SQM QoS via mainline sqm-scripts (uci `sqm`, one `queue` section on br-lan).
// DEGRADE: the GL interlocks are MediaTek/GL-specific and absent on the Siflower
// device, so they are dropped:
//   - gl_dpi_qos "please turn off qos" guard (config never exists on mainline)
//   - gl_eqos / uci `qos` teardown + reload (GL C-based qos engine, absent)
//   - MTK hw-NAT offload disable (gl-session get/set_netnat_config, MTK-only)
// As the mainline-correct replacement for the hw-offload interlock we optionally
// turn off fw4 flow_offloading when enabling SQM (offloaded flows bypass the
// qdisc shaper). Everything else is ported faithfully, INCLUDING the one-arm
// upload/download SWAP on br-lan: SQM shapes on the LAN bridge, so ingress/egress
// invert versus the user's WAN-oriented input.
'use strict';

export default {
	get_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let res = {};
		let found = false;
		c.foreach("sqm", "queue", function(s) {
			if (found) return;   // first queue section wins (orig `return false`)
			res.enable = (s.enabled == "1");
			res.qdisc = s.qdisc;
			let upload = int(s.upload ?? 0);
			let download = int(s.download ?? 0);
			if (type(upload) != "int" || upload < 0) upload = 0;      // NaN -> double -> 0
			if (type(download) != "int" || download < 0) download = 0;
			if (upload < 1000 && upload >= 500) upload = 1000;
			else if (upload < 500) upload = 0;
			if (download < 1000 && download >= 500) download = 1000;
			else if (download < 500) download = 0;
			res.upload = upload == 0 ? "" : upload / 1000.0;
			res.download = download == 0 ? "" : download / 1000.0;
			// br-lan is the LAN bridge: shaper egress = user download, shaper
			// ingress = user upload, so present them swapped back to the app.
			if (s.interface == "br-lan") {
				res.download = upload == 0 ? "" : upload / 1000.0;
				res.upload = download == 0 ? "" : download / 1000.0;
			}
			found = true;
		});
		if (!found) {
			res.enable = false;
			res.qdisc = "cake";
			res.upload = "";
			res.download = "";
		}
		return res;
	},

	set_config: function(args, ctx) {
		let c = ctx.uci.cursor();

		// DEGRADE: gl_dpi_qos "please turn off the qos" guard dropped (config absent).

		if (args.qdisc != "cake" && args.qdisc != "fq_codel")
			return ctx.util.E.INVALID_PARAMS;
		if (type(args.enable) != "bool")
			return ctx.util.E.INVALID_PARAMS;
		if (type(args.upload) != "int" && type(args.upload) != "double" && type(args.upload) != "string")
			return ctx.util.E.INVALID_PARAMS;
		if (type(args.download) != "int" && type(args.download) != "double" && type(args.download) != "string")
			return ctx.util.E.INVALID_PARAMS;

		let upload = +args.upload;      // Mbit; string|number -> number, NaN on junk
		let download = +args.download;
		if (!upload || !download)       // NaN / 0 -> falsy
			return ctx.util.E.INVALID_PARAMS;
		if (upload < 1 || upload > 10000 || download < 1 || download > 10000)
			return ctx.util.E.INVALID_PARAMS;

		// DEGRADE: MTK hw-NAT offload disable (get/set_netnat_config) dropped.
		// Mainline-correct replacement: when enabling SQM, turn off fw4 flow
		// offloading so shaped flows are not offloaded past the qdisc.
		if (args.enable) {
			let changed = false;
			if (c.get("firewall", "@defaults[0]", "flow_offloading") == "1") {
				c.set("firewall", "@defaults[0]", "flow_offloading", "0");
				changed = true;
			}
			if (c.get("firewall", "@defaults[0]", "flow_offloading_hw") == "1") {
				c.set("firewall", "@defaults[0]", "flow_offloading_hw", "0");
				changed = true;
			}
			if (changed) {
				c.commit("firewall");
				ctx.popen("/etc/init.d/firewall reload >/dev/null 2>&1");
			}
		}

		// DEGRADE: gl_eqos / uci `qos` teardown + reload dropped (GL qos engine absent).

		let up_kbit = int(upload * 1000);
		let dn_kbit = int(download * 1000);

		let found = false;
		c.foreach("sqm", "queue", function(s) {
			if (found) return;   // first queue section wins (orig `return false`)
			let name = s[".name"];
			c.set("sqm", name, "enabled", args.enable ? "1" : "0");
			c.set("sqm", name, "qdisc", args.qdisc);
			c.set("sqm", name, "script", args.qdisc == "fq_codel" ? "simple.qos" : "piece_of_cake.qos");
			c.set("sqm", name, "upload", up_kbit);
			c.set("sqm", name, "download", dn_kbit);
			// br-lan SWAP: shaper upload = user download, shaper download = user upload.
			if (s.interface == "br-lan") {
				c.set("sqm", name, "upload", dn_kbit);
				c.set("sqm", name, "download", up_kbit);
			}
			found = true;
		});

		if (!found) {
			let name = c.add("sqm", "queue");
			c.set("sqm", name, "interface", "br-lan");
			c.set("sqm", name, "enabled", args.enable ? "1" : "0");
			c.set("sqm", name, "qdisc", args.qdisc);
			// new section is on br-lan -> written SWAPPED (upload = user download).
			c.set("sqm", name, "upload", dn_kbit);
			c.set("sqm", name, "download", up_kbit);
			c.set("sqm", name, "script", args.qdisc == "fq_codel" ? "simple.qos" : "piece_of_cake.qos");
			c.set("sqm", name, "qdisc_advanced", "0");
			c.set("sqm", name, "ingress_ecn", "ECN");
			c.set("sqm", name, "egress_ecn", "ECN");
			c.set("sqm", name, "qdisc_really_really_advanced", "0");
			c.set("sqm", name, "itarget", "auto");
			c.set("sqm", name, "etarget", "auto");
			c.set("sqm", name, "linklayer", "none");
		}

		c.commit("sqm");
		ctx.popen("sync");
		ctx.popen("/etc/init.d/sqm restart >/dev/null 2>&1");
		return {};
	}
};
