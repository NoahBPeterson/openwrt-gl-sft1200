{%
// gl-ucode: uhttpd ucode-handler binding for POST /rpc.
//
// IMPORTANT: uhttpd compiles handler files as ucode TEMPLATES, not scripts/modules.
// The whole file must live inside a `{% ... %}` code block, and uhttpd dispatches to
// a `handle_request(env)` function it finds in the VM scope (so assign it via
// `global.handle_request = ...`). `uhttpd` is an ambient global uhttpd injects
// (send/sendc/recv/urldecode/urlencode/docroot). `import` works inside the template.
// Wire up:  list ucode_prefix '/rpc=/usr/share/gl-ucode/handler.uc'
'use strict';

import { handle } from './rpc.uc';

/**
 * @param {object} env
 * @param {string} name
 */
function header(env, name) {
	if (env.headers && env.headers[name] != null) return env.headers[name];
	return env['HTTP_' + uc(replace(name, /-/g, '_'))];
}

/**
 * @param {object} env
 */
global.handle_request = function(env) {
	if (env.REQUEST_METHOD != 'POST') {
		uhttpd.send('Status: 405 Method Not Allowed\r\nContent-Type: application/json\r\n\r\n');
		uhttpd.send('{"jsonrpc":"2.0","id":null,"error":{"code":-32600,"message":"Invalid Request"}}');
		return;
	}

	let remote = env.REMOTE_ADDR ?? '';
	// the app authenticates `call`s via the Admin-Token cookie (params[0] left empty)
	let cookie = header(env, 'cookie') ?? '';
	let tm = match(cookie, /Admin-Token=([^;[:space:]]+)/);
	let reqinfo = {
		remote_addr: remote,
		is_local: (remote == '127.0.0.1' || remote == '::1'),
		glinet: (header(env, 'glinet') != null),
		token: tm ? tm[1] : null
	};

	let sink = {};
	let resp = handle(uhttpd.recv() ?? '', reqinfo, sink);

	uhttpd.send('Status: 200 OK\r\nContent-Type: application/json\r\nX-Frame-Options: DENY\r\n');
	if (sink.set_cookie)
		uhttpd.send('Set-Cookie: ' + sink.set_cookie + '\r\n');
	uhttpd.send('\r\n' + sprintf('%J', resp));
};
%}
