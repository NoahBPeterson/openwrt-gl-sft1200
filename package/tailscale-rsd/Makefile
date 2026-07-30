# Native-Rust Tailscale client (GeiserX/tailscale-rs) built from source for OpenWrt.
#
# Why this exists: the Go tailscaled is far too heavy for a 32-bit MIPS SoC (load average >11 on a
# GL-SFT1200). This builds the Rust client with the kernel-WireGuard hybrid data plane, which hands
# direct-peer crypto to the kernel's wg interface and leaves only relay peers in userspace
# (measured load on the same device: 0.39).
#
# The upstream tree has no glinet_tsd daemon and no kernel-wg feature yet; both live in patches/
# until they land upstream, at which point the patches can simply be dropped.

include $(TOPDIR)/rules.mk

PKG_NAME:=tailscale-rsd
PKG_VERSION:=0.43.0
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/GeiserX/tailscale-rs.git
PKG_SOURCE_VERSION:=4a20cda8d1cf06c26fd7a01518518ba95bc29616
PKG_SOURCE_DATE:=2026-07-28
PKG_MIRROR_HASH:=skip

PKG_MAINTAINER:=sft1200-port
PKG_LICENSE:=BSD-3-Clause
PKG_LICENSE_FILES:=LICENSE

PKG_BUILD_DEPENDS:=rust/host
# MIPS16 and Rust codegen do not mix.
PKG_BUILD_FLAGS:=no-mips16
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(TOPDIR)/package/lang/rust/rust-package.mk

# Hybrid data plane: kernel wg for direct peers, userspace + DERP for relayed ones.
RUST_PKG_FEATURES:=kernel-wg

# The daemon is an example, not a [[bin]]: it uses clap and tracing-subscriber, which upstream
# carries as dev-dependencies (available to examples, not to bins). `cargo install --example`
# installs it into $(PKG_INSTALL_DIR)/bin all the same.
define Build/Compile
	$(call Build/Compile/Cargo,,--example glinet_tsd)
endef

define Package/tailscale-rsd
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=VPN
  TITLE:=Tailscale client (native Rust, kernel-WireGuard hybrid)
  URL:=https://github.com/GeiserX/tailscale-rs
  DEPENDS:=$(RUST_ARCH_DEPENDS) +kmod-wireguard
endef

define Package/tailscale-rsd/description
  A Tailscale client written in Rust, small enough for low-power routers. Runs as a procd
  service, keeps a persistent node key in /etc/tailscale-rs/node.json and publishes its state
  to /var/run/tailscale-rs/status.json for the LuCI page and the GL.iNet app backend to read.

  Direct-endpoint peers are programmed into a kernel WireGuard interface (wg-ts) so packet
  crypto runs in the kernel; relay-only peers stay on the userspace path.
endef

define Package/tailscale-rsd/conffiles
/etc/tailscale-rs/node.json
endef

# rust-package.mk's default install expects /usr/bin/<bin>; the service (and the ucode backend
# that drives it) both expect /usr/sbin/tailscale-rsd, so install it under that name.
define Package/tailscale-rsd/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/bin/glinet_tsd $(1)/usr/sbin/tailscale-rsd
endef

$(eval $(call BuildPackage,tailscale-rsd))
