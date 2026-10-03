#!/usr/bin/env python3
"""Builds a TOFS (TheOS File System) image from a directory tree.

Layout (all offsets relative to the start of this image):
  sector 0        : superblock - magic "TOFS", uint32 entry_count
  sectors 1-5      : directory - up to 64 entries, 40 bytes each:
                      name[24], uint32 type (0=file, 1=dir),
                      int32 parent (entry index, or -1 for the root),
                      uint32 start_lba (absolute disk LBA, files only),
                      uint32 size_bytes (files only)
  sector 6+        : file data, one file after another, each file
                      starting on a sector boundary
"""
import sys
import struct
import pathlib

SECTOR_SIZE = 512
DIR_SECTORS = 5
RECORD_SIZE = 40
MAX_ENTRIES = (DIR_SECTORS * SECTOR_SIZE) // RECORD_SIZE
NAME_LEN = 24

TYPE_FILE = 0
TYPE_DIR = 1
ROOT_PARENT = -1


def walk(dir_path, parent_index, entries):
    if not dir_path.is_dir():
        return
    for p in sorted(dir_path.iterdir(), key=lambda p: p.name):
        name = p.name.encode("ascii", "strict")
        if len(name) >= NAME_LEN:
            print(f"error: name too long: {p.name}", file=sys.stderr)
            sys.exit(1)
        if p.is_dir():
            index = len(entries)
            entries.append({"name": name, "type": TYPE_DIR, "parent": parent_index,
                             "start_lba": 0, "size": 0})
            walk(p, index, entries)
        elif p.is_file():
            content = p.read_bytes()
            entries.append({"name": name, "type": TYPE_FILE, "parent": parent_index,
                             "start_lba": 0, "size": len(content), "content": content})


def path_of(entries, index):
    parts = []
    while index != ROOT_PARENT:
        parts.append(entries[index]["name"].decode())
        index = entries[index]["parent"]
    return "/" + "/".join(reversed(parts))


def main():
    if len(sys.argv) != 4:
        print(f"usage: {sys.argv[0]} <output.img> <fs_dir> <fs_base_lba>", file=sys.stderr)
        return 1

    out_path = sys.argv[1]
    fs_dir = pathlib.Path(sys.argv[2])
    fs_base_lba = int(sys.argv[3])

    entries = []
    walk(fs_dir, ROOT_PARENT, entries)

    if len(entries) > MAX_ENTRIES:
        print(f"error: too many entries ({len(entries)} > {MAX_ENTRIES})", file=sys.stderr)
        return 1

    data_start_lba = fs_base_lba + 1 + DIR_SECTORS
    data = bytearray()
    next_lba = data_start_lba

    for e in entries:
        if e["type"] == TYPE_FILE:
            e["start_lba"] = next_lba
            content = e["content"]
            data += content
            pad = (-len(content)) % SECTOR_SIZE
            data += b"\x00" * pad
            next_lba += (len(content) + pad) // SECTOR_SIZE

    superblock = b"TOFS" + struct.pack("<I", len(entries))
    superblock += b"\x00" * (SECTOR_SIZE - len(superblock))

    directory = bytearray(DIR_SECTORS * SECTOR_SIZE)
    for i, e in enumerate(entries):
        off = i * RECORD_SIZE
        directory[off:off + len(e["name"])] = e["name"]
        struct.pack_into("<I", directory, off + NAME_LEN, e["type"])
        struct.pack_into("<i", directory, off + NAME_LEN + 4, e["parent"])
        struct.pack_into("<I", directory, off + NAME_LEN + 8, e["start_lba"])
        struct.pack_into("<I", directory, off + NAME_LEN + 12, e["size"])

    with open(out_path, "wb") as f:
        f.write(superblock)
        f.write(directory)
        f.write(data)

    for i, e in enumerate(entries):
        kind = "DIR " if e["type"] == TYPE_DIR else "FILE"
        suffix = "/" if e["type"] == TYPE_DIR else ""
        print(f"  {kind} {path_of(entries, i) + suffix:<30} size={e['size']}")
    total_sectors = 1 + DIR_SECTORS + len(data) // SECTOR_SIZE
    print(f"TOFS image: {len(entries)} entries, {total_sectors} sectors")
    return 0


if __name__ == "__main__":
    sys.exit(main())
