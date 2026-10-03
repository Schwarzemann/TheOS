#!/usr/bin/env bash
# Assembles the boot, stage 2, kernel and fs images into one flat disk image.
set -euo pipefail

BUILD_DIR=${1:?usage: build_image.sh <build-dir>}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/layout.sh"

BOOT_BIN="$BUILD_DIR/boot.bin"
STAGE2_BIN="$BUILD_DIR/stage2.bin"
KERNEL_BIN="$BUILD_DIR/kernel.bin"
FS_IMG="$BUILD_DIR/fs.img"
IMG="$BUILD_DIR/TheOS.img"

boot_size=$(stat -c%s "$BOOT_BIN")
if [ "$boot_size" -ne "$SECTOR_SIZE" ]; then
    echo "error: boot.bin must be exactly $SECTOR_SIZE bytes (got $boot_size)" >&2
    exit 1
fi

stage2_max=$(( STAGE2_SECTORS * SECTOR_SIZE ))
stage2_size=$(stat -c%s "$STAGE2_BIN")
if [ "$stage2_size" -gt "$stage2_max" ]; then
    echo "error: stage2.bin too large ($stage2_size > $stage2_max bytes)" >&2
    exit 1
fi

kernel_max=$(( KERNEL_SECTORS * SECTOR_SIZE ))
kernel_size=$(stat -c%s "$KERNEL_BIN")
if [ "$kernel_size" -gt "$kernel_max" ]; then
    echo "error: kernel.bin too large ($kernel_size > $kernel_max bytes)" >&2
    exit 1
fi

rm -f "$IMG"
cp "$BOOT_BIN" "$IMG"
truncate -s "$SECTOR_SIZE" "$IMG"

dd if="$STAGE2_BIN" of="$IMG" bs="$SECTOR_SIZE" seek="$STAGE2_LBA" conv=notrunc status=none
truncate -s $(( KERNEL_LBA * SECTOR_SIZE )) "$IMG"

dd if="$KERNEL_BIN" of="$IMG" bs="$SECTOR_SIZE" seek="$KERNEL_LBA" conv=notrunc status=none
truncate -s $(( FS_BASE_LBA * SECTOR_SIZE )) "$IMG"

fs_size=$(stat -c%s "$FS_IMG")
fs_sectors=$(( fs_size / SECTOR_SIZE ))
dd if="$FS_IMG" of="$IMG" bs="$SECTOR_SIZE" seek="$FS_BASE_LBA" conv=notrunc status=none
truncate -s $(( (FS_BASE_LBA + fs_sectors) * SECTOR_SIZE )) "$IMG"

echo "Built $IMG ($(stat -c%s "$IMG") bytes)"
