# Building the GL-SFT1200 image

This fork carries the GL.iNet GL-SFT1200 (Siflower SF19A2890) target support plus the packages
that make up the firmware. Images are built locally — there is deliberately no CI image build.

## 1. Feeds

```sh
./scripts/feeds update -a
./scripts/feeds install -a
```

`package/lang/rust` is a local override (Rust 1.96.0, copied from the packages feed's master
branch). The openwrt-25.12 packages feed still ships 1.94.0, which is a hair below the Tailscale
client's 1.94.1 MSRV. `feeds install` skips packages that already exist under `package/`, so the
override wins; do not re-create `package/feeds/packages/rust`.

## 2. Config

```sh
cp sft1200.diffconfig .config
make defconfig
```

`sft1200.diffconfig` is the diffconfig of the shipping image: the SFT1200 target, the out-of-tree
`sf_smac` WiFi driver, LuCI, WireGuard, adblock-fast and `glinet-ucode` (which pulls in
`tailscale-rsd`).

## 3. Build

```sh
make -j$(nproc)
```

The first build compiles rustc from source for the `tailscale-rsd` package, which takes a while.
Output lands in `bin/targets/siflower/sf19a2890/`.

## Packages

| package | source |
| --- | --- |
| `glinet-ucode` | https://github.com/NoahBPeterson/glinet-ucode (pinned commit) |
| `tailscale-rsd` | https://github.com/GeiserX/tailscale-rs (pinned commit) + patches in `package/tailscale-rsd/patches/` |

Neither is vendored here. To pick up changes, bump `PKG_SOURCE_VERSION` (and `PKG_SOURCE_DATE`) in
the relevant Makefile.

The `tailscale-rsd` patches add 32-bit mipsel support, the kernel-WireGuard hybrid data plane and
the `glinet_tsd` daemon, none of which are upstream yet. When they land upstream, drop the patches
and bump the pin.

## Local build tweaks

`rules.mk` (`COMMITCOUNT`) and `scripts/getver.sh` are sometimes patched locally to pin a version
string when building from a shallow tree. Those changes are intentionally **not** committed — they
would break version stamping for everyone else.
