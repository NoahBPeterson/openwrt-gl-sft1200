# GL.iNet app backend, reimplemented natively in ucode for mainline OpenWrt 25.12.
# Pure ucode + already-shipped runtime (uhttpd/ucode/ubus/uci/fs/digest) — no lua,
# no nginx, no eco. Serves the GL.iNet app's /rpc JSON-RPC on the existing uhttpd.

include $(TOPDIR)/rules.mk

PKG_NAME:=glinet-ucode
PKG_VERSION:=0.1.0
PKG_RELEASE:=1
PKG_MAINTAINER:=sft1200-port
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define Package/glinet-ucode
  SECTION:=gl-inet
  CATEGORY:=GL.iNet
  TITLE:=GL.iNet app /rpc backend (ucode, mainline-native)
  URL:=https://github.com/ (sft1200-port)
  # uhttpd-mod-ucode gives the native ucode handler (preferred). It's a hard dep so
  # the production image bundles it; postinst still keeps a CGI fallback (/www/rpc)
  # in case a slimmed image drops the module.
  # shadow-chpasswd: ui.set_password uses `chpasswd -c MD5`; busybox's chpasswd
  # lacks -c, so the full shadow-utils chpasswd is required for onboarding.
  # adblock-fast: backs the switch-button "adguardhome" toggle with a light dnsmasq
  # blocklist (the maintained package; the handler falls back to our own list if absent).
  DEPENDS:= \
	+uhttpd +uhttpd-mod-ucode \
	+ucode +ucode-mod-fs +ucode-mod-uci +ucode-mod-ubus +ucode-mod-digest \
	+shadow-chpasswd \
	+adblock-fast
  PKGARCH:=all
endef

define Package/glinet-ucode/description
  Native ucode reimplementation of the GL.iNet router RPC backend (gl-sdk4 / oui)
  so the GL.iNet mobile app and web console work against a stock mainline 25.12
  router. Implements the /rpc JSON-RPC protocol (challenge/login/logout/alive/call),
  the gl-session auth algorithm, and all 39 RPC objects (system/network/wifi/clients/
  ui + lan/firewall/dns/netmode/cable/repeater/vpn-client/upgrade/tor/zerotier/
  tailscale/adguardhome/sqm/ksmbd/wg_client/kmwan/qos/ipv6/igmp/led/timer/diag/
  logread/tethering/black_white_list/local-access/acl/switch-button/edgerouter/
  ovpn_client/luci/dpi/flow_statistics/parental-control). Objects backed by absent
  daemons/hardware degrade gracefully to sane defaults.
endef

define Build/Compile
endef

define Package/glinet-ucode/install
	$(CP) ./files/* $(1)/
	chmod 0755 $(1)/usr/libexec/gl-ucode/rpc-cgi
	chmod 0755 $(1)/www/rpc
endef

define Package/glinet-ucode/postinst
#!/bin/sh
[ -n "$${IPKG_INSTROOT}" ] && exit 0
# Wire POST /rpc into the running uhttpd 'main' instance. Prefer the native ucode
# handler if uhttpd-mod-ucode is present; otherwise fall back to the CGI endpoint
# (executable /www/rpc, reached via cgi_prefix).
if uci -q get uhttpd.main >/dev/null; then
	if [ -e /usr/lib/ucode/uhttpd.so ] || opkg list-installed 2>/dev/null | grep -q uhttpd-mod-ucode; then
		uci -q del_list uhttpd.main.ucode_prefix='/rpc=/usr/share/gl-ucode/handler.uc'
		uci add_list uhttpd.main.ucode_prefix='/rpc=/usr/share/gl-ucode/handler.uc'
	else
		# CGI fallback: only executable files under cgi_prefix run as CGI; /rpc -> /www/rpc.
		uci set uhttpd.main.cgi_prefix='/rpc'
	fi
	uci commit uhttpd
	/etc/init.d/uhttpd reload >/dev/null 2>&1
fi
mkdir -p /tmp/gl-ucode
exit 0
endef

$(eval $(call BuildPackage,glinet-ucode))
