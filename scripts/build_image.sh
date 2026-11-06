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

fs_size=$(stat -c%s "$FS_IMG")
img_size=$(( FS_BASE_LBA * SECTOR_SIZE + fs_size ))

# The TOFS region of an existing image holds files written inside the OS, so
# keep it unless fs.img changed (fs/ or the format was edited) or it can't be
# this layout's filesystem.
keep_fs=0
if [ -f "$IMG" ] && [ ! "$FS_IMG" -nt "$IMG" ] && [ "$(stat -c%s "$IMG")" -eq "$img_size" ] &&
   [ "$(dd if="$IMG" bs=1 skip=$(( FS_BASE_LBA * SECTOR_SIZE )) count=4 status=none)" = "TOFS" ]; then
    keep_fs=1
fi

if [ "$keep_fs" -eq 1 ]; then
    echo "Keeping the filesystem in $IMG (make reset-fs to start again from fs/)"
    dd if=/dev/zero of="$IMG" bs="$SECTOR_SIZE" count="$FS_BASE_LBA" conv=notrunc status=none
else
    if [ -f "$IMG" ]; then
        echo "Replacing the filesystem in $IMG from fs/: files written inside the OS are discarded"
    fi
    rm -f "$IMG"
    truncate -s "$img_size" "$IMG"
    dd if="$FS_IMG" of="$IMG" bs="$SECTOR_SIZE" seek="$FS_BASE_LBA" conv=notrunc status=none
fi

dd if="$BOOT_BIN" of="$IMG" bs="$SECTOR_SIZE" seek="$BOOT_LBA" conv=notrunc status=none
dd if="$STAGE2_BIN" of="$IMG" bs="$SECTOR_SIZE" seek="$STAGE2_LBA" conv=notrunc status=none
dd if="$KERNEL_BIN" of="$IMG" bs="$SECTOR_SIZE" seek="$KERNEL_LBA" conv=notrunc status=none

echo "Built $IMG ($(stat -c%s "$IMG") bytes)"
