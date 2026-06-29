REQUIRE_IMAGE_METADATA=1

platform_check_image() {
	return 0
}

platform_do_upgrade() {
	# raw "kernel" mtd + UBI "ubi" mtd (names match nand.sh CI_KERNPART/CI_UBIPART defaults)
	nand_do_upgrade "$1"
}
