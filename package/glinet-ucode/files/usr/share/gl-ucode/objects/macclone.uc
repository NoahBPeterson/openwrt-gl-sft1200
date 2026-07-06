// gl-ucode object: macclone  (app's "MAC Address" page -> macclone.get_mac)
// The stock object is gl-sdk4-macclone (reads lfactory + gl_macclone uci). On
// mainline we source the device MAC from the LAN device / factory uci and report
// the same field names the app reads. set_mac clones the WAN MAC via uci+netifd.
'use strict';

const fs = require('fs');

function readfile(p) { let d = fs.readfile(p); return d ? trim(d) : null; }

function dev_mac() {
	for (let dev in ["br-lan", "eth0", "eth0.1"]) {
		let a = readfile(`/sys/class/net/${dev}/address`);
		if (a) return uc(a);
	}
	return "";
}

export default {
	get_mac: function(args, ctx) {
		let c = ctx.uci.cursor();
		let factory = c.get("factory", "device", "mac");
		let base = factory ? uc(factory) : dev_mac();
		// current WAN mac (may be cloned); fall back to the base mac
		let wan_mac = readfile("/sys/class/net/eth0.2/address")
			?? readfile("/sys/class/net/wan/address") ?? base;
		return {
			macaddr: base,
			mac: base,
			factory_mac: base,
			secondwan_mac: "",
			remote_mac: uc(wan_mac),
			repeater_mac: ""
		};
	},

	// Clone (override) the WAN MAC. Stored in uci network.wan.macaddr; netifd applies
	// it on reload. Empty/factory value clears the override.
	set_mac: function(args, ctx) {
		if (type(args.mac) != "string" || !match(args.mac, /^[0-9a-fA-F]{2}(:[0-9a-fA-F]{2}){5}$/))
			return ctx.util.E.INVALID_PARAMS;
		let c = ctx.uci.cursor();
		c.set("network", "wan", "macaddr", lc(args.mac));
		c.commit("network");
		ctx.popen("/etc/init.d/network reload >/dev/null 2>&1");
		return {};
	}
};
