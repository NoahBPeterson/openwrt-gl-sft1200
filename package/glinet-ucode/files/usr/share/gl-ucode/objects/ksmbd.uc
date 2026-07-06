// gl-ucode object: ksmbd  (port of decompiled-spec/rpc/ksmbd.lua, 4.3.25)
// SMB/Samba file sharing on stock ksmbd-server (uci `ksmbd`: `config share` /
// `config user`, read_only/guest_ok/read_list/write_list/users, top-level enable).
// Share/user CRUD = pure uci + `ksmbd.adduser`; disk enum shells df/blkid + walks
// /mnt; WAN toggle edits a `sambasharewan` firewall rule + `bind interfaces only`.
// DEGRADE: GL used stock ksmbd almost unmodified, so nothing GL-specific is dropped.
// When ksmbd/config/tools are ABSENT the getters return empty lists/sane defaults
// (foreach over a missing config iterates nothing; ctx.popen of a missing binary
// returns ""), and the set_* methods no-op their init.d/adduser side effects.
// GL runtime shims replaced: ngx.timer.at(apply) -> backgrounded init.d restart;
// os.execute/io.popen -> ctx.popen; oui.fs -> fs; oui.ubus -> ctx.ubus.
'use strict';

const fs = require('fs');

// Lua gmatch("[^sep]+") semantics: split on sep, drop empty tokens (collapses runs).
function tokens(str, sep) {
	if (str == null) return null;
	let out = [];
	for (let t in split(str, sep)) if (t != "") push(out, t);
	return out;
}

function basename(p) {
	let m = match(p, /([^\/]+)$/);
	return m ? m[1] : p;
}

// restart ksmbd out-of-band, but only when it is enabled (mirrors the original apply()).
function apply(ctx) {
	let c = ctx.uci.cursor();
	if (c.get("ksmbd", "enable") == "1")
		ctx.popen("(sleep 1; /etc/init.d/ksmbd restart) >/dev/null 2>&1 &");
}

// build read/write/all user CSVs from a [{name, readonly}] list, exactly as the
// original: read_list = readonly==1 users, write_list = the rest, all = write,read.
function user_lists(users) {
	let read_list = null, write_list = null;
	for (let user in (users ?? [])) {
		if (user.readonly == 1)
			read_list = read_list == null ? user.name : read_list + "," + user.name;
		else if (write_list == null)
			write_list = user.name;
		else
			write_list = write_list + "," + user.name;
	}
	let all_list = (write_list ?? "") + (read_list != null ? "," + read_list : "");
	return { read: read_list, write: write_list, all: all_list };
}

// firewall `sambasharewan` rule: open 445 tcpudp from wan when wan_access==1, else drop.
// Option names (dest_port/dest_proto) are kept as the GL config had them.
function reload_firewall(ctx, wan_access) {
	let c = ctx.uci.cursor();
	if (wan_access == 1) {
		c.set("firewall", "sambasharewan", "rule");
		c.set("firewall", "sambasharewan", "src", "wan");
		c.set("firewall", "sambasharewan", "dest_port", "445");
		c.set("firewall", "sambasharewan", "dest_proto", "tcpudp");
		c.set("firewall", "sambasharewan", "target", "ACCEPT");
	} else {
		c.delete("firewall", "sambasharewan");
	}
	c.commit("firewall");
	ctx.ubus.call("service", "event", { type: "config.change", data: { package: "firewall" } });
}

function count_subdirs(path) {
	let count = 0;
	for (let file in (fs.lsdir(path) ?? [])) {
		if (index(file, ".") == 0) continue;   // skip dotfiles (Lua ^%.)
		let s = fs.stat(path + "/" + file);
		if (s && s.type == "directory") count++;
	}
	return count;
}

function get_subdir_counts(path) {
	let files = [];
	for (let file in (fs.lsdir(path) ?? [])) {
		if (index(file, ".") == 0) continue;
		let fpath = path + "/" + file;
		let s = fs.stat(fpath);
		if (s && s.type == "directory") {
			let n = replace(fpath, /^\/mnt/, "");
			n = replace(n, /\/$/, "");
			push(files, { have_dir: count_subdirs(fpath), n: n });
		}
	}
	return files;
}

// remove `user` from a comma CSV, returning the trimmed CSV or "" when it empties.
function del_user(all_user, user) {
	if (all_user == null || all_user == "") return "";
	let out = [];
	for (let m in split(all_user, ",")) if (m != "" && m != user) push(out, m);
	return length(out) == 0 ? "" : join(",", out);
}

export default {
	// Faithful to the original: the RPC object being present means ksmbd is available.
	get_status: function(args, ctx) {
		return { enable: 1 };
	},

	set_share: function(args, ctx) {
		if (args.proto != "samba") return {};
		let c = ctx.uci.cursor();
		let share_id = args.share_id;
		c.set("ksmbd", share_id, "name", args.share_name);
		c.set("ksmbd", share_id, "read_only", args.public_readonly == 1 ? "yes" : "no");
		c.set("ksmbd", share_id, "guest_ok", args.public == 1 ? "yes" : "no");
		if (args.users) {
			let ul = user_lists(args.users);
			if (ul.read != null) c.set("ksmbd", share_id, "read_list", ul.read);
			else c.delete("ksmbd", share_id, "read_list");
			if (ul.write != null) c.set("ksmbd", share_id, "write_list", ul.write);
			else c.delete("ksmbd", share_id, "write_list");
			c.set("ksmbd", share_id, "users", ul.all);
		} else {
			c.delete("ksmbd", share_id, "read_list");
			c.delete("ksmbd", share_id, "write_list");
			c.delete("ksmbd", share_id, "users");
		}
		c.commit("ksmbd");
		apply(ctx);
		return {};
	},

	get_disk_list: function(args, ctx) {
		let result = {
			disk_number: 1,
			disk: [ { part: [], sd_card: 0, part_num: 0 } ]
		};
		let data = ctx.popen("df -m | grep /dev/sd");
		let sds = tokens(data, "\n");
		if (sds) {
			for (let line in sds) {
				let sd = tokens(line, " ");
				if (!sd || length(sd) < 6) continue;
				let bd = ctx.popen("blkid " + sd[0]);
				if (bd == null) continue;
				let mu = match(bd, /UUID="([^"]+)"/);
				let mt = match(bd, /TYPE="([^"]+)"/);
				if (!mt) continue;                       // no filesystem type -> skip (orig behaviour)
				push(result.disk[0].part, {
					fs_type: uc(mt[1]),
					dev_name: basename(sd[0]),
					label: basename(sd[0]),
					disk_name: basename(sd[5]),
					free_size: sd[3],
					total_len: sd[1],
					uid: mu ? mu[1] : null
				});
				result.disk[0].part_num++;
			}
		}
		return result;
	},

	eject_disk: function(args, ctx) {
		let data = ctx.popen("df -m | grep /dev/sd");
		let sds = tokens(data, "\n");
		let find_it = false;
		if (sds) {
			for (let line in sds) {
				let sd = tokens(line, " ");
				if (!sd || length(sd) < 6) continue;
				if (sd[0] == "/dev/" + args.dev_name) {
					// sd[0]/sd[5] come from df output, not from the caller -> safe to shell.
					ctx.popen("/etc/init.d/ksmbd stop; umount -l " + sd[0] + " && rm -r " + sd[5]);
					find_it = true;
					break;
				}
			}
		}
		if (!find_it)
			return { err_code: -1, err_msg: "no find dev name" };
		return {};
	},

	set_proto_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		for (let proto in (args.protos ?? [])) {
			if (proto.name != "samba") continue;
			c.set("ksmbd", "enable", `${proto.enable}`);
			let bind = proto.wan_access == 1 ? "no" : "yes";
			let action = proto.enable == 1 ? "start" : "stop";
			let cmd = sprintf("sed -i 's/bind interfaces only =.\\+/bind interfaces only = %s/' /etc/ksmbd/smb.conf.template; /etc/init.d/ksmbd %s", bind, action);
			reload_firewall(ctx, proto.wan_access);
			ctx.popen(cmd);
			c.commit("ksmbd");
		}
		return {};
	},

	get_proto_config: function(args, ctx) {
		let c = ctx.uci.cursor();
		let enable = c.get("ksmbd", "enable") == "1" ? 1 : 0;
		let wan_access = 0;
		let conf = fs.readfile("/etc/ksmbd/smb.conf");
		if (conf) {
			for (let line in split(conf, "\n")) {
				let m = match(line, /bind interfaces only = (\S+)/);
				if (m) wan_access = m[1] == "no" ? 1 : 0;
			}
		}
		return {
			res: [ { enable: enable, name: "samba", share_type: 1, wan_access: wan_access } ]
		};
	},

	get_file_list: function(args, ctx) {
		let path = "/mnt" + args.path;
		if (index(path, "../") >= 0 || index(path, "/..") >= 0 || index(path, "'") >= 0)
			return { err_code: -32602, err_msg: "Invalid params" };
		if (!fs.access(path))
			return { err_code: -2, err_msg: "no find path" };
		return { files: get_subdir_counts(path), result_code: 0 };
	},

	get_share_list: function(args, ctx) {
		let c = ctx.uci.cursor();
		let share = [];
		c.foreach("ksmbd", "share", function(s) {
			let users = [];
			if (s.read_list || s.write_list) {
				let rl = tokens(s.read_list, ",");
				if (rl) for (let u in rl) push(users, { name: u, readonly: 1 });
				let wl = tokens(s.write_list, ",");
				if (wl) for (let u in wl) push(users, { name: u, readonly: 0 });
			}
			push(share, {
				n: replace(s.path ?? "", /^\/mnt/, ""),
				owner_readonly: 0,
				protos: [ {
					public: s.guest_ok == "yes" ? 1 : 0,
					public_readonly: s.read_only == "yes" ? 1 : 0,
					enable: 1,
					share_name: s.name,
					name: "samba",
					users: users
				} ],
				share_id: s[".name"],
				file_ok: (s.path && fs.access(s.path)) ? 1 : 0,
				t: "d"
			});
		});
		return { share: share, result_code: 0 };
	},

	add_share: function(args, ctx) {
		if (args.proto != "samba") return {};
		let c = ctx.uci.cursor();
		let file_path = "/mnt" + args.file;
		if (index(file_path, "../") >= 0 || index(file_path, "/..") >= 0 || index(file_path, "'") >= 0)
			return { err_code: -32602, err_msg: "Invalid params" };
		if (!fs.access(file_path))
			return { err_code: -2, err_msg: "no find path" };

		let i = 0, exist = false;
		c.foreach("ksmbd", "share", function(s) {
			i++;
			if (!s.path || s.path == file_path) exist = true;
		});
		if (exist || i > 9)   // 10-share cap
			return { err_code: -1, err_msg: "share dir already exists or exceeds limits" };

		let sid = c.add("ksmbd", "share");
		if (args.users) {
			let ul = user_lists(args.users);
			c.set("ksmbd", sid, "users", ul.all);
			if (ul.read != null) c.set("ksmbd", sid, "read_list", ul.read);
			if (ul.write != null) c.set("ksmbd", sid, "write_list", ul.write);
		}
		c.set("ksmbd", sid, "name", args.share_name);
		c.set("ksmbd", sid, "path", file_path);
		c.set("ksmbd", sid, "read_only", args.public_readonly == 1 ? "yes" : "no");
		c.set("ksmbd", sid, "guest_ok", args.public == 1 ? "yes" : "no");
		c.set("ksmbd", sid, "create_mask", "0666");
		c.set("ksmbd", sid, "dir_mask", "0777");
		c.commit("ksmbd");
		apply(ctx);
		return {};
	},

	remove_share: function(args, ctx) {
		let c = ctx.uci.cursor();
		if (args.all == 1) {
			c.foreach("ksmbd", "share", function(s) { c.delete("ksmbd", s[".name"]); });
			c.commit("ksmbd");
			apply(ctx);
		} else if (args.proto == "samba") {
			c.delete("ksmbd", args.share_id);
			c.commit("ksmbd");
			apply(ctx);
		}
		return {};
	},

	add_user: function(args, ctx) {
		let c = ctx.uci.cursor();
		let name = args.name, password = args.password;
		if (!(name && password) || index(name, "'") >= 0 || index(password, "'") >= 0)
			return { err_code: -32602, err_msg: "Invalid params" };

		let i = 0, exist = false;
		c.foreach("ksmbd", "user", function(s) {
			i++;
			if (s.name == name) exist = true;
		});
		if (exist || i > 9)   // 10-user cap
			return { err_code: -1, err_msg: "user already exists or exceeds limits" };

		let sid = c.add("ksmbd", "user");
		c.set("ksmbd", sid, "name", name);
		c.set("ksmbd", sid, "password", password);
		c.commit("ksmbd");
		ctx.popen(sprintf("ksmbd.adduser -a '%s' -p '%s'", name, password));
		return {};
	},

	remove_user: function(args, ctx) {
		let c = ctx.uci.cursor();
		let name = args.name;
		if (args.all != 1 && (!name || index(name, "'") >= 0))
			return { err_code: -32602, err_msg: "Invalid params" };

		if (args.all == 1) {
			c.foreach("ksmbd", "user", function(s) { c.delete("ksmbd", s[".name"]); });
			c.foreach("ksmbd", "share", function(ss) {
				c.delete("ksmbd", ss[".name"], "read_list");
				c.delete("ksmbd", ss[".name"], "write_list");
				c.delete("ksmbd", ss[".name"], "users");
			});
			ctx.popen("> /etc/ksmbd/ksmbdpwd.db");
		} else {
			c.foreach("ksmbd", "user", function(s) {
				if (s.name != name) return;
				c.delete("ksmbd", s[".name"]);
				ctx.popen("ksmbd.adduser -d '" + s.name + "'");
				c.foreach("ksmbd", "share", function(ss) {
					let rl = del_user(ss.read_list, s.name);
					let wl = del_user(ss.write_list, s.name);
					let al = del_user(ss.users, s.name);
					if (al == "") {
						c.delete("ksmbd", ss[".name"], "read_list");
						c.delete("ksmbd", ss[".name"], "write_list");
						c.delete("ksmbd", ss[".name"], "users");
					} else {
						c.set("ksmbd", ss[".name"], "read_list", rl);
						c.set("ksmbd", ss[".name"], "write_list", wl);
						c.set("ksmbd", ss[".name"], "users", al);
					}
				});
			});
		}
		c.commit("ksmbd");
		apply(ctx);
		return {};
	},

	set_user_pwd: function(args, ctx) {
		let c = ctx.uci.cursor();
		let name = args.name, password = args.password;
		if (!(name && password) || index(name, "'") >= 0 || index(password, "'") >= 0)
			return { err_code: -32602, err_msg: "Invalid params" };
		c.foreach("ksmbd", "user", function(s) {
			if (s.name == name) c.set("ksmbd", s[".name"], "password", password);
		});
		ctx.popen(sprintf("ksmbd.adduser -u '%s' -p '%s'", name, password));
		c.commit("ksmbd");
		apply(ctx);
		return {};
	},

	get_user_list: function(args, ctx) {
		let c = ctx.uci.cursor();
		let list = [];
		c.foreach("ksmbd", "user", function(s) {
			push(list, { name: s.name, password: s.password });
		});
		return { list: list };
	}
};
