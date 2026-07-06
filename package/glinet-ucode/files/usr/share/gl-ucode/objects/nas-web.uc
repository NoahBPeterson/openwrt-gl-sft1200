// gl-ucode object: nas-web (network storage). Degrades to "no disk" — a full NAS (samba/webdav/
// dlna over a mounted disk) is an image-integration feature; the `ksmbd` object covers SMB shares.
'use strict';

export default {
	get_status:       function(args, ctx) { return { enable: false, disk_number: 0, disk: [] }; },
	get_proto_config: function(args, ctx) { return { samba: false, webdav: false, dlna: false, ftp: false }; }
};
