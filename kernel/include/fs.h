#ifndef THEOS_FS_H
#define THEOS_FS_H

#include <stdint.h>

/* Reader for TOFS (TheOS File System), the tiny read-only format
 * scripts/mkfs_tofs.py bakes onto disk right after the kernel image */

#define FS_NAME_LEN 24
#define FS_MAX_ENTRIES 64

/* Absolute LBA where the fs.img region starts on disk. Must match
 * FS_BASE_LBA in scripts/layout.sh. */
#define FS_BASE_LBA 261

#define FS_TYPE_FILE 0
#define FS_TYPE_DIR  1

#define FS_ROOT      ((int32_t)-1)
#define FS_NOT_FOUND ((int32_t)-2)

typedef struct {
    char name[FS_NAME_LEN];
    uint32_t type;
    int32_t parent;      /* index into the entry table, or FS_ROOT */
    uint32_t start_lba;  /* files only */
    uint32_t size_bytes; /* files only */
} fs_entry_t;

void fs_init(void);
uint32_t fs_entry_count(void);
const fs_entry_t *fs_entry_at(uint32_t index);

/* True for FS_ROOT and for entries of type FS_TYPE_DIR. */
int fs_is_dir(int32_t index);

/* Looks up a direct child of `dir` (FS_ROOT for the root) by name.
 * Returns its entry index, or FS_NOT_FOUND. */
int32_t fs_find_child(int32_t dir, const char *name);

/* Resolves `path` (absolute, or relative to `from`), handling "." and "..".
 * Writes the result to *out_index and returns 1, or 0 if a component is missing. */
int fs_resolve(int32_t from, const char *path, int32_t *out_index);

/* Reads an entry's full contents into buf, which must be at least
 * fs_sectors(entry) * 512 bytes. Returns the file size in bytes. */
uint32_t fs_read(const fs_entry_t *entry, void *buf);
uint32_t fs_sectors(const fs_entry_t *entry);

#endif
