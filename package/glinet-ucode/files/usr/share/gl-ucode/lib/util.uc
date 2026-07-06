// gl-ucode: shared utilities (error codes, JSON-RPC response builders, helpers)
// ES module — import { E, error_response, ... } from './util.uc'.
'use strict';

import { open, readfile } from 'fs';

// JSON-RPC error codes (verbatim from oui/rpc.lua)
export const E = {
	NONE:                  0,
	PARSE_ERROR:           -32700,
	INVALID_REQUEST:       -32600,
	METHOD_NOT_FOUND:      -32601,
	INVALID_PARAMS:        -32602,
	INTERNAL_ERROR:        -32603,
	ACCESS:                -32000,
	NOT_FOUND:             -32001,
	SESSION_OVER_LIMIT:    -32002,
	LOGIN_FAIL_OVER_LIMIT: -32003
};

const E_MSG = {
	"-32700": "Parse error",
	"-32600": "Invalid Request",
	"-32601": "Method not found",
	"-32602": "Invalid params",
	"-32603": "Internal error",
	"-32000": "Access denied",
	"-32001": "Not found",
	"-32002": "Login session's number over limit",
	"-32003": "Login fail number over limit"
};

const ALPHABET = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/**
 * @param {integer|null} id
 * @param {integer} code
 */
export function error_response(id, code, data) {
	return {
		jsonrpc: "2.0",
		id: (id != null) ? id : null,
		error: { code: code, message: E_MSG["" + code] ?? "Unknown", data: data }
	};
};

/**
 * @param {integer} id
 * @param {object|array} result
 */
export function result_response(id, result) {
	return {
		jsonrpc: "2.0",
		id: (id != null) ? id : null,
		result: (result != null) ? result : null
	};
};

// cryptographically-random id of n chars from ALPHABET (urandom)
/**
 * @param {number} n
 */
export function generate_id(n) {
	let f = open("/dev/urandom", "r");
	let raw = f ? f.read(n) : null;
	if (f) f.close();
	let out = "";
	let alen = length(ALPHABET);
	for (let i = 0; i < n; i++) {
		let b = raw ? ord(raw, i) : int(rand() % 256);
		out += substr(ALPHABET, b % alen, 1);
	}
	return out;
};

// read /etc/shadow, return { field, alg, salt } for a username, or null
/**
 * @param {string} username
 */
export function shadow_entry(username) {
	if (!username || !match(username, /^[a-z][-a-z0-9_]*$/))
		return null;
	let data = readfile(getenv("GL_UCODE_SHADOW") ?? "/etc/shadow");
	if (!data) return null;
	for (let line in split(data, "\n")) {
		let m = match(line, /^([^:]+):([^:]+)/);
		if (m && m[1] == username) {
			let field = m[2];                       // full $alg$salt$hash
			if (field == null)
				continue;
			let p = match(field, /^\$([0-9a-z]+)\$([^$]+)\$/);
			return { field: field, alg: p ? p[1] : null, salt: p ? p[2] : null };
		}
	}
	return null;
};

/**
 * @param {string} s
 */
export function trim(s) {
	return s ? replace(s, /^\s+|\s+$/g, "") : s;
};
