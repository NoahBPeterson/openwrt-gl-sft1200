KERNEL_LOADADDR := 0x80100000

define Device/Default
  PROFILES := Default
  BLOCKSIZE := 64k
  FILESYSTEMS := squashfs
  DEVICE_DTS_DIR := ../dts
  KERNEL := kernel-bin | append-dtb | lzma | uImage lzma
  KERNEL_INITRAMFS := kernel-bin | append-dtb | lzma | uImage lzma
  IMAGES := sysupgrade.bin
  IMAGE/sysupgrade.bin = append-kernel | pad-to $$$$(BLOCKSIZE) | \
	append-rootfs | pad-rootfs | append-metadata
endef

define Device/siflower_sf19a2890-evb
  DEVICE_VENDOR := Siflower
  DEVICE_MODEL := SF19A2890 EVB
  BOARD_NAME := siflower,sf19a2890-evb
  DEVICE_DTS := sf19a2890_evb
  DEVICE_PACKAGES := kmod-sf_smac
endef
TARGET_DEVICES += siflower_sf19a2890-evb

# GL.iNet GL-SFT1200 "Opal" — SF19A2890, SPI-NAND, RTL8367C switch, siwifi WiFi.
# Stock GL/U-Boot expects a tar sysupgrade laid out as
# sysupgrade-glinet_gl-sft1200/{CONTROL,kernel,root} — produced by sysupgrade-tar.
define Device/glinet_gl-sft1200
  DEVICE_VENDOR := GL.iNet
  DEVICE_MODEL := GL-SFT1200
  BOARD_NAME := glinet_gl-sft1200
  DEVICE_DTS := sf19a2890_glinet_gl-sft1200
  # SPI-NAND: 128k erase block, 2k page (from live /proc/mtd erasesize 0x20000)
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  KERNEL := kernel-bin | append-dtb | lzma | uImage lzma
  IMAGES := sysupgrade.bin
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
  DEVICE_PACKAGES := kmod-sf_smac
endef
TARGET_DEVICES += glinet_gl-sft1200
