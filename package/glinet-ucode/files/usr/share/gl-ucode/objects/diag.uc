// gl-ucode object: diag  (port of decompiled-spec/rpc/diag.lua)
// Network diagnostics (ping/traceroute). Shells to busybox ping/traceroute via
// ctx.popen instead of ngx.pipe.spawn. No GL-only sources; nothing dropped.
'use strict';

// Faithful port of Lua isValidIPv4: four dot-separated decimal octets, each 0-255.
function isValidIPv4(ip) {
	if (!ip)
		return false;
	if (!match(ip, /^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$/))
		return false;
	for (let octet in split(ip, ".")) {
		let num = int(octet);
		if (num < 0 || num > 255)
			return false;
	}
	return true;
}

// Faithful port of Lua isValidDomain: trim, length 1-253, only [a-zA-Z0-9-.],
// no leading/trailing/consecutive dots, >=2 labels, each label 1-63 chars with
// no leading/trailing hyphen and a valid character set.
function isValidDomain(domain) {
	if (!domain)
		return false;

	// Remove leading and trailing spaces
	domain = trim(domain);
	let len = length(domain);
	if (len < 1 || len > 253)
		return false;

	// Check for illegal characters (only letters, numbers, hyphens, and dots)
	if (match(domain, /[^a-zA-Z0-9.-]/))
		return false;

	// Check for leading/trailing dots and consecutive dots
	if (substr(domain, 0, 1) == "." || substr(domain, -1) == "." || match(domain, /\.\./))
		return false;

	// Split labels and validate
	let labels = split(domain, ".");
	if (length(labels) < 2)
		return false;  // At least two labels are required (e.g., example.com)

	for (let label in labels) {
		let label_len = length(label);
		// Check label length
		if (label_len < 1 || label_len > 63)
			return false;
		// Check hyphen position
		if (match(label, /^-/) || match(label, /-$/))
			return false;
		// Check valid character combination
		if (!match(label, /^[a-zA-Z0-9][a-zA-Z0-9-]*$/))
			return false;
	}

	return true;
}

export default {
	// test network connection: busybox `ping -c4 <addr>`
	ping: function(args, ctx) {
		let addr = args.addr;
		if (!isValidIPv4(addr) && !isValidDomain(addr))
			return { ping_result: "ip address is not correct" };
		let str = ctx.popen(`ping -c4 ${addr} 2>&1`);
		if (!str || length(str) == 0)
			str = "destination host unreachable";
		return { ping_result: str };
	},

	// trace network path: busybox `traceroute -4 -q 1 -w 1 -n <addr>`
	traceroute: function(args, ctx) {
		let addr = args.addr;
		if (!isValidIPv4(addr) && !isValidDomain(addr))
			return { trace_result: "ip address is not correct" };
		let str = ctx.popen(`traceroute -4 -q 1 -w 1 -n ${addr} 2>&1`);
		if (!str || length(str) == 0)
			str = "destination host unreachable";
		return { trace_result: str };
	}
};
