// gl-ucode object: acl  (port of decompiled-spec/rpc/acl.lua)
// oui-httpd user/permission store: groups, per-group ACL entries, and users.
// The original persisted everything in SQLite (/etc/oui/oui.db via lsqlite3): one
// dynamically-created table `acl_<group>` per group (rows scope/entry/perm) plus an
// `account(username, group)` table. Our ucode framework has NO sqlite binding, so
// this is retargeted onto a plain uci store `acl` with three section types:
//   config group      name=<group>
//   config acl_entry  group=<group> scope=<s> entry=<e> perm=<p>
//   config user       name=<username> group=<group>
// The 8 methods round-trip over that store (add/remove write sections; get_* read
// them back). Method names, param names, app-facing field names (`groups`, `acls`,
// each acl's scope/entry/perm) and the {err_code,err_msg} shapes are preserved.
// The valid_group_name / valid_username validators are ported byte-faithfully.
//
// DEGRADE: this is only the config store the app's admin-permission UI reads/writes.
// Actual permission ENFORCEMENT is out of scope here -- our rpc.uc access() model is
// simpler (root always allowed; local + glinet callers allowed; scoped per-group ACL
// is not implemented), so nothing consults these sections at auth time. Dropping a
// group also drops its acl_entry sections (mirrors the original DROP TABLE acl_<g>).
'use strict';

// valid_group_name: faithful port of the Lua validator (security-sensitive).
//   must be a non-empty string, start with a letter or '_', and contain only
//   alphanumerics, '-' or '_'. (Lua: ^[%a_] and no [^%w%-_].)
function valid_group_name(group) {
	if (type(group) != "string" || group == "")
		return false;
	if (!match(group, /^[a-zA-Z_]/))
		return false;
	if (match(group, /[^a-zA-Z0-9_-]/))   // dash last + unescaped (ucode regex gotcha)
		return false;
	return true;
}

// valid_username: faithful port -- ^[a-z][-a-z0-9_]*$ (dash moved last, unescaped).
function valid_username(username) {
	return type(username) == "string" && match(username, /^[a-z][a-z0-9_-]*$/) != null;
}

function group_exists(c, group) {
	let found = false;
	c.foreach("acl", "group", function(s) {
		if (s.name == group) found = true;
	});
	return found;
}

// section id (.name) of the acl_entry matching group/scope/entry, or null.
function find_acl(c, group, scope, entry) {
	let id = null;
	c.foreach("acl", "acl_entry", function(s) {
		if (s.group == group && s.scope == scope && s.entry == entry)
			id = s[".name"];
	});
	return id;
}

// section id (.name) of the user matching username, or null.
function find_user(c, username) {
	let id = null;
	c.foreach("acl", "user", function(s) {
		if (s.name == username) id = s[".name"];
	});
	return id;
}

export default {
	get_group_list: function(args, ctx) {
		let c = ctx.uci.cursor();
		let groups = [];
		c.foreach("acl", "group", function(s) {
			if (s.name != null) push(groups, s.name);
		});
		return { groups: groups };
	},

	add_group: function(args, ctx) {
		let group = args.group;
		if (!valid_group_name(group))
			return ctx.util.E.INVALID_PARAMS;
		let c = ctx.uci.cursor();
		if (group_exists(c, group))
			return { err_code: -1, err_msg: "exists" };
		let sid = c.add("acl", "group");
		c.set("acl", sid, "name", group);
		c.commit("acl");
		return {};
	},

	remove_group: function(args, ctx) {
		let group = args.group;
		if (!valid_group_name(group))
			return ctx.util.E.INVALID_PARAMS;
		let c = ctx.uci.cursor();
		if (!group_exists(c, group))
			return { err_code: -1, err_msg: "not exists" };
		// Drop the group section and every acl_entry it owned (mirrors DROP TABLE).
		let victims = [];
		c.foreach("acl", "group", function(s) {
			if (s.name == group) push(victims, s[".name"]);
		});
		c.foreach("acl", "acl_entry", function(s) {
			if (s.group == group) push(victims, s[".name"]);
		});
		for (let id in victims)
			c.delete("acl", id);
		c.commit("acl");
		return {};
	},

	get_acl_list: function(args, ctx) {
		let group = args.group;
		if (!valid_group_name(group))
			return ctx.util.E.INVALID_PARAMS;
		let c = ctx.uci.cursor();
		let acls = [];
		c.foreach("acl", "acl_entry", function(s) {
			if (s.group == group)
				push(acls, { scope: s.scope, entry: s.entry, perm: s.perm });
		});
		return { acls: acls };
	},

	add_acl: function(args, ctx) {
		let group = args.group;
		let scope = args.scope;
		let entry = args.entry;
		let perm = args.perm;
		if (!valid_group_name(group))
			return ctx.util.E.INVALID_PARAMS;
		if (type(scope) != "string" || type(entry) != "string" || type(perm) != "string")
			return ctx.util.E.INVALID_PARAMS;
		let c = ctx.uci.cursor();
		// Original INSERTed even on duplicate then reported "exists"; to keep the
		// store clean we skip the write on duplicate but preserve the error shape.
		if (find_acl(c, group, scope, entry) != null)
			return { err_code: -1, err_msg: "exists" };
		let sid = c.add("acl", "acl_entry");
		c.set("acl", sid, "group", group);
		c.set("acl", sid, "scope", scope);
		c.set("acl", sid, "entry", entry);
		c.set("acl", sid, "perm", perm);
		c.commit("acl");
		return {};
	},

	remove_acl: function(args, ctx) {
		let group = args.group;
		let scope = args.scope;
		let entry = args.entry;
		if (!valid_group_name(group))
			return ctx.util.E.INVALID_PARAMS;
		if (type(scope) != "string" || type(entry) != "string")
			return ctx.util.E.INVALID_PARAMS;
		let c = ctx.uci.cursor();
		let id = find_acl(c, group, scope, entry);
		if (id == null)
			return { err_code: -1, err_msg: "not exists" };
		c.delete("acl", id);
		c.commit("acl");
		return {};
	},

	add_user: function(args, ctx) {
		let group = args.group;
		let username = args.username;
		if (type(group) != "string" || !valid_username(username))
			return ctx.util.E.INVALID_PARAMS;
		let c = ctx.uci.cursor();
		// Replace any existing account for this username (original DELETE then INSERT).
		let id = find_user(c, username);
		if (id != null)
			c.delete("acl", id);
		let sid = c.add("acl", "user");
		c.set("acl", sid, "name", username);
		c.set("acl", sid, "group", group);
		c.commit("acl");
		return {};
	},

	remove_user: function(args, ctx) {
		let username = args.username;
		if (!valid_username(username))
			return ctx.util.E.INVALID_PARAMS;
		let c = ctx.uci.cursor();
		let id = find_user(c, username);
		if (id == null)
			return { err_code: -1, err_msg: "not exists" };
		c.delete("acl", id);
		c.commit("acl");
		return {};
	}
};
