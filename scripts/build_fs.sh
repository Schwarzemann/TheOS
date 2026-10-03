#!/usr/bin/env bash
# Builds build/fs.img, the TOFS image stored on disk right after the kernel.
set -euo pipefail

BUILD_DIR=${1:?usage: build_fs.sh <build-dir> <fs-dir>}
FS_DIR=${2:?usage: build_fs.sh <build-dir> <fs-dir>}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/layout.sh"

mkdir -p "$BUILD_DIR"
python3 "$SCRIPT_DIR/mkfs_tofs.py" "$BUILD_DIR/fs.img" "$FS_DIR" "$FS_BASE_LBA"
