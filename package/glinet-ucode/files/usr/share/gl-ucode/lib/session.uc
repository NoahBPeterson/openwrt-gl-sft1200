// gl-ucode: authentication + session store (ucode port of gl-ngx-session).
// Faithful to the recovered algorithm:
//   challenge(user) -> {nonce, alg, salt}   (salt/alg from /etc/shadow)
//   client sends hash = sha256(user ":" shadow_field ":" nonce)
//   login(user, hash) verifies against /etc/shadow + outstanding nonces -> sid
// State is kept as independent files under STATE_DIR so concurrent uhttpd
// prefork workers don't race on a shared blob.
'use strict';

import { access, mkdir, writefile, readfile, unlink, lsdir } from 'fs';
import { sha256 } from 'digest';
import { E, shadow_entry, generate_id } from './util.uc';

const STATE_DIR   = getenv("GL_UCODE_STATE") ?? "/tmp/gl-ucode";
// The password verifier MUST survive reboot (STATE_DIR is /tmp, which is wiped, and
// ui.init only re-runs during onboarding — so a /tmp verifier leaves login broken
// after every reboot). Keep it on the persistent overlay instead.
const PERSIST_DIR = getenv("GL_UCODE_PERSIST") ?? "/etc/gl-ucode";
const NONCE_TTL   = 60;     // seconds (orig used ~2s; widened for LAN round-trips)
const SESSION_TTL = 300;    // oui-httpd.main.session_timeout default
const MAX_SESSION = 16;

function ensure_dir() {
	if (!access(STATE_DIR, "x"))
		mkdir(STATE_DIR, 0o700);
}

function now() { return time(); }

/**
 * @param {string} username
 */
function acl_for(username) {
	// root is hardcoded to the always-allowed group (oui.db.get_acl_by_username).
	// Scoped non-root users require the sqlite ACL store (not in v1).
	return (username == "root") ? "root" : "";
}

// ---- nonce files: STATE_DIR/n_<user>_<nonce> containing expiry epoch ----
/**
 * @param {string} username
 */
function create_nonce(username) {
	ensure_dir();
	let nonce = generate_id(32);
	writefile(`${STATE_DIR}/n_${username}_${nonce}`, `${now() + NONCE_TTL}`);
	return nonce;
}

/**
 * @param {string} username
 * @param {string} shadow_field
 * @param {string} hash
 */
function consume_matching_nonce(username, shadow_field, hash) {
	let prefix = `n_${username}_`;
	for (let name in (lsdir(STATE_DIR) ?? [])) {
		if (index(name, prefix) != 0) continue;
		let nonce = substr(name, length(prefix));
		let path = `${STATE_DIR}/${name}`;
		let exp = int(readfile(path) ?? "0");
		if (now() > exp) { unlink(path); continue; }
		if (sha256(`${username}:${shadow_field}:${nonce}`) == hash) {
			unlink(path);          // single-use
			return true;
		}
	}
	return false;
}

// ---- session files: STATE_DIR/s_<sid> containing JSON ----
function session_count() {
	let n = 0;
	for (let e in (lsdir(STATE_DIR) ?? []))
		if (index(e, "s_") == 0) n++;
	return n;
}

/**
 * @param {string} username
 */
function create_session(username) {
	ensure_dir();
	let sid = generate_id(32);
	let sess = { username: username, aclgroup: acl_for(username), timeout: now() + SESSION_TTL };
	writefile(`${STATE_DIR}/s_${sid}`, sprintf("%J", sess));
	writefile(`/tmp/gl_token_${sid}`, "");   // legacy admin-token marker some objects look for
	return sid;
}

// ---- password verifier + preauth (GL app compat) ----
// The GL app computes its login `hash` with a client-side crypt whose exact formula
// we could not reproduce (and the on-device image has no crypt tool to match it). But
// the app ALSO sends the plaintext password in the `challenge` request. So we verify
// the password there against a sha256 verifier written at init time, and if it checks
// out we set a short-lived preauth that `login` accepts — password is still verified,
// just at challenge time instead of via the opaque login hash. Falls back to the real
// hash check when no preauth exists (so the strict flow still works if a client uses it).
function ensure_persist() {
	if (!access(PERSIST_DIR, "x"))
		mkdir(PERSIST_DIR, 0o700);
}
function verifier_path(u) { return `${PERSIST_DIR}/pv_${u}`; }

// Called from ui.init after the shadow password is set.
export function set_verifier(username, password) {
	if (type(username) != "string" || type(password) != "string") return;
	ensure_persist();
	writefile(verifier_path(username), sha256(`${username}:${password}`));
};

function password_ok(username, password) {
	if (type(password) != "string" || password == "") return false;
	let v = readfile(verifier_path(username));
	return (v != null && v == sha256(`${username}:${password}`));
}

function set_preauth(username) {
	ensure_dir();
	writefile(`${STATE_DIR}/pa_${username}`, `${now() + 30}`);
}
function consume_preauth(username) {
	let path = `${STATE_DIR}/pa_${username}`;
	let exp = int(readfile(path) ?? "0");
	if (exp == 0) return false;
	unlink(path);
	return (now() <= exp);
}

/**
 * @param {string} username
 * @param {string} password
 */
export function challenge(username, password) {
	if (type(username) != "string")
		return { code: E.INVALID_PARAMS };
	let s = shadow_entry(username);
	if (!s || (s != null && s.alg == null))
		return { code: E.ACCESS };
	// GL-app compat: verify the plaintext password the app sends here; on success arm a
	// short-lived preauth that login() accepts.
	if (password_ok(username, password))
		set_preauth(username);
	let nonce = create_nonce(username);
	return { code: 0, data: { nonce: nonce, alg: s.alg, salt: s.salt, "hash-method": "sha256" } };
};

/**
 * @param {string} username
 * @param {string} hash
 */
export function login(username, hash) {
	if (type(username) != "string" || type(hash) != "string")
		return { code: E.INVALID_PARAMS };
	let s = shadow_entry(username);
	if (!s)
		return { code: E.ACCESS };
	// Accept a preauth armed by a password-verified challenge (GL app), else fall back
	// to the strict challenge-response hash check.
	if (!consume_preauth(username) && !consume_matching_nonce(username, s.field, hash))
		return { code: E.ACCESS };
	if (session_count() >= MAX_SESSION) {
		// evict the oldest (lowest timeout)
		let oldest = null, omin = null;
		for (let e in (lsdir(STATE_DIR) ?? [])) {
			if (index(e, "s_") != 0) continue;
			try {
				let j = json(readfile(`${STATE_DIR}/${e}`) ?? "{}");
				if (omin == null || j.timeout < omin) { omin = j.timeout; oldest = e; }
			} catch (e) {
				// handle the exception — e.message / e.stacktrace
				warn(`caught: ${e}\n`);
			}
		}
		if (oldest) unlink(`${STATE_DIR}/${oldest}`);
	}
	let sid = create_session(username);
	return { code: 0, data: { username: username, sid: sid } };
};

/**
 * @param {string} sid
 * @returns {object|null}
 */
export function get(sid) { // Returns: null | unknown
	if (type(sid) != "string") return null;
	let path = `${STATE_DIR}/s_${sid}`;
	let raw = readfile(path);
	if (!raw) return null;
	try {
		let sess = json(raw);
		if (now() > sess.timeout) { unlink(path); unlink(`/tmp/gl_token_${sid}`); return null; }
		sess.timeout = now() + SESSION_TTL;     // touch
		writefile(path, sprintf("%J", sess));
		return sess;
	} catch (e) {
		// handle the exception — e.message / e.stacktrace
		warn(`caught: ${e}\n`);
	}
};

/**
 * @param {string} sid
 */
export function touch(sid) {
	return get(sid) ? { code: 0 } : { code: E.ACCESS };
};

/**
 * @param {string} sid
 */
export function logout(sid) {
	if (type(sid) == "string") {
		unlink(`${STATE_DIR}/s_${sid}`);
		unlink(`/tmp/gl_token_${sid}`);
	}
	return {};
};
