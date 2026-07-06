// gl-ucode object: tethering  (port of decompiled-spec/rpc/tethering.lua)
// USB/phone tethering as WAN: detect the tether netdev by USB driver signature
// (/sys/class/net/*/device/uevent DRIVER=), bind it as network.tethering (proto
// dhcp), add it to the wan firewall zone, set metric/mtu/ttl, and reload.
// DEGRADE: the GL /proc/gl-hw-info/usb-* gating + hardware.platform_* helpers are
// replaced by direct sysfs walks; the vSIM (simo)/OTG/gl_otg config is DROPPED;
// the kmwan metric default (30) is a plain constant; glconfig.manual_dns branch
// dropped (DNS taken straight from netifd's dns-server list).
'use strict';

const fs = require('fs');

const ERR_TETHERING_UNSUPPORT     = -1;
const ERR_TETHERING_NO_DEVICE     = -2;
const ERR_TETHERING_PARAMS_INVALID = -3;

const METRIC_DEFAULT = "30";   // was kmwan.tethering.metric on GL devices

function readfile(p) { let d = fs.readfile(p); return d ? d : null; }

// Android tether: rndis_host, or cdc_ncm from Google's vendor id (18d1).
function check_is_android_device(uevent) {
	if (match(uevent, /rndis_host/)) return true;
	if (match(uevent, /cdc_ncm/)) {
		let m = match(uevent, /PRODUCT=([0-9a-fA-F]+)\//);
		if (m && m[1] == "18d1") return true;
	}
	return false;
}

// Is this netdev a USB tether device, judged purely by its driver uevent?
function check_is_tethering_device(ifname) {
	let path = `/sys/class/net/${ifname}/device/uevent`;
	if (!fs.access(path)) return false;
	let uevent = readfile(path);
	if (!uevent) return false;
	if (match(uevent, /ipheth/) || match(uevent, /rndis_host/) || match(uevent, /cdc_ncm/))
		return true;
	// cdc_ether: exclude the common serial-bridge vendors (CH34x 1a86, 0fe6)
	if (match(uevent, /cdc_ether/)) {
		let m = match(uevent, /PRODUCT=([0-9a-fA-F]+)\//);
		let vid = m ? m[1] : null;
		if (vid != "1a86" && vid != "fe6") return true;
	}
	return false;
}

// type: 1 = iPhone (ipheth), 0 = Android/USB gadget, 2 = other/unknown
function classify(ifname) {
	if (match(ifname, /^eth[0-9]/)) {
		let uevent = readfile(`/sys/class/net/${ifname}/device/uevent`) ?? "";
		if (match(uevent, /ipheth/)) return 1;
		if (check_is_android_device(uevent)) return 0;
		return 2;
	}
	if (match(ifname, /^usb[0-9]/)) return 0;
	return 2;
}

// Scan /sys/class/net for present tether devices (driver-signature detection).
function scan_devices() {
	let devices = [];
	for (let ifname in (fs.lsdir("/sys/class/net") ?? [])) {
		if (match(ifname, /^wwan/) || ifname == "simonet") continue;
		if (!check_is_tethering_device(ifname)) continue;
		push(devices, { device: ifname, type: classify(ifname), use: false });
	}
	return devices;
}

// Append "tethering" to the wan firewall zone's network list (idempotent).
function add_to_wan_zone(c) {
	c.foreach("firewall", "zone", function(s) {
		if (s.name != "wan") return;
		let nets = s.network ?? [];
		if (type(nets) == "string") nets = split(trim(nets), /[ \t\n]+/);
		if (type(nets) != "array") nets = [];
		for (let n in nets) if (n == "tethering") return;
		push(nets, "tethering");
		c.set("firewall", s[".name"], "network", nets);
	});
}

// mtu/ttl/ttl_ipv6 enforcement. Target is OpenWrt 25.12 (fw4/nftables), so ttl is
// applied via drop-in .nft files in the mangle_postrouting chain.
function set_tethering_params(c, ctx, device, mtu, ttl, ttl_ipv6) {
	if (mtu != null)
		c.set("network", "tethering", "mtu", `${mtu}`);

	if (ttl != null || ttl_ipv6 != null) {
		if (ttl != null) c.set("network", "tethering", "ttl", `${ttl}`);
		if (ttl_ipv6 != null) c.set("network", "tethering", "ttl_ipv6", `${ttl_ipv6}`);
	} else {
		c.delete("network", "tethering", "ttl");
		c.delete("network", "tethering", "ttl_ipv6");
	}

	let nftdir = "/usr/share/nftables.d/chain-pre/mangle_postrouting";
	if (fs.access("/sbin/fw4")) {
		if (ttl != null) {
			ctx.popen(`mkdir -p ${nftdir}`);
			fs.writefile(`${nftdir}/01-tethering-set-ttl.nft`,
				sprintf("oifname %s ip ttl set %d\n", device, int(ttl)));
		} else {
			ctx.popen(`rm -f ${nftdir}/01-tethering-set-ttl.nft`);
		}
		if (ttl_ipv6 != null) {
			ctx.popen(`mkdir -p ${nftdir}`);
			fs.writefile(`${nftdir}/01-tethering-set-ttl_ipv6.nft`,
				sprintf("oifname %s ip6 nexthdr != ipv6-icmp ip6 hoplimit != 255 ip6 hoplimit set %d\n", device, int(ttl_ipv6)));
		} else {
			ctx.popen(`rm -f ${nftdir}/01-tethering-set-ttl_ipv6.nft`);
		}
	}

	add_to_wan_zone(c);
	c.commit("network");
	c.commit("firewall");
}

// number in [min,max]; null passes (optional param)
function valid_num(v, lo, hi) {
	if (v == null) return true;
	if (type(v) != "int" && type(v) != "double") return false;
	return v >= lo && v <= hi;
}

export default {
	get_status: function(args, ctx) {
		let c = ctx.uci.cursor();
		let devices = scan_devices();
		let res = { devices: devices, status: 0 };

		let s = ctx.ubus.call("network.interface.tethering", "status") ?? {};
		if (s.up == null) return res;

		let v4 = s["ipv4-address"];
		if (v4 && length(v4) > 0) {
			res.status = 1;
			res.ipv4 = { ip: `${v4[0].address}/${v4[0].mask}`, dns: [] };
			for (let r in (s.route ?? []))
				if (r.target == "0.0.0.0") res.ipv4.gateway = r.nexthop;
			if (s.device)
				for (let dev in res.devices)
					if (s.device == dev.device) dev.use = true;
			// DEGRADE: glconfig.manual_dns path dropped -> use netifd's dns-server list
			res.ipv4.dns = s["dns-server"] ?? [];
		} else {
			// interface exists but no lease yet: configured+enabled -> connecting (2)
			if (c.get("network", "tethering", "ifname") &&
			    c.get("network", "tethering", "disabled") != "1")
				res.status = 2;
			else
				res.status = 0;
			return res;
		}

		let s6 = ctx.ubus.call("network.interface.tethering6", "status") ?? {};
		if (s6.up) {
			res.ipv6 = {};
			let v6 = s6["ipv6-address"] ?? [];
			if (length(v6) > 0)
				res.ipv6.ip = `${v6[0].address}/${v6[0].mask}`;
			for (let r in (s6.route ?? []))
				if (r.target == "::") res.ipv6.gateway = r.nexthop;
			res.ipv6.dns = s6["dns-server"] ?? [];
		}

		if (length(devices) == 0)
			return { devices: devices, status: res.status };
		return res;
	},

	get_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let mtu = c.get("network", "tethering", "mtu");
		let ttl = c.get("network", "tethering", "ttl");
		let ttl6 = c.get("network", "tethering", "ttl_ipv6");
		return {
			device:   c.get("network", "tethering", "ifname"),
			mtu:      mtu != null ? int(mtu) : null,
			ttl:      ttl != null ? int(ttl) : null,
			ttl_ipv6: ttl6 != null ? int(ttl6) : null
		};
	},

	set_connect: function(args, ctx) {
		let c = ctx.uci.cursor();
		let device = args.device;
		let mtu = args.mtu, ttl = args.ttl, ttl_ipv6 = args.ttl_ipv6;

		if (!valid_num(mtu, 576, 9200))
			return { err_code: ERR_TETHERING_PARAMS_INVALID, err_msg: "params invalid" };
		if (!valid_num(ttl, 1, 255))
			return { err_code: ERR_TETHERING_PARAMS_INVALID, err_msg: "params invalid" };
		if (!valid_num(ttl_ipv6, 1, 255))
			return { err_code: ERR_TETHERING_PARAMS_INVALID, err_msg: "params invalid" };
		if (!device || device == "")
			return { err_code: ERR_TETHERING_NO_DEVICE, err_msg: "No device" };
		// DEGRADE: vSIM (network.interface.simo) conflict check dropped (no simo on mainline)

		let metric = c.get("kmwan", "tethering", "metric") ?? METRIC_DEFAULT;

		c.delete("network", "tethering");
		c.set("network", "tethering", "interface");
		c.set("network", "tethering", "proto", "dhcp");
		c.set("network", "tethering", "ifname", device);
		c.set("network", "tethering", "metric", `${metric}`);
		c.commit("network");

		set_tethering_params(c, ctx, device, mtu, ttl, ttl_ipv6);
		// DROP: gl_otg `tethering` uci store (OTG-only auto/mtu/ttl mirror)

		ctx.popen("sync");
		ctx.ubus.call("network", "reload");
		ctx.popen("/etc/init.d/firewall reload >/dev/null 2>&1");
		return {};
	},

	disconnect: function(args, ctx) {
		let c = ctx.uci.cursor();
		c.set("network", "tethering", "disabled", "1");
		c.commit("network");
		// DROP: gl_otg `tethering` auto=0 mirror (OTG-only)
		ctx.popen("sync");
		ctx.ubus.call("network", "reload");
		return {};
	}
};
