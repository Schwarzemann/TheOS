#ifndef THEOS_FS_H
#define THEOS_FS_H

#include <stdint.h>

/* Reader/writer for TOFS (TheOS File System), the tiny format
 * scripts/mkfs_tofs.py bakes onto disk right after the kernel image. */

#define FS_NAME_LEN 24
#define FS_MAX_ENTRIES 64
#define FS_MAX_FREE_EXTENTS 32

/* Absolute LBA where the fs.img region starts on disk. Must match
 * FS_BASE_LBA in scripts/layout.sh. */
#define FS_BASE_LBA 261

/* Total sectors reserved for the whole TOFS region (superblock + directory
 * + file data + free space) starting at FS_BASE_LBA. Must match
 * FS_TOTAL_SECTORS in scripts/layout.sh. */
#define FS_TOTAL_SECTORS 8192

#define FS_TYPE_FILE 0
#define FS_TYPE_DIR  1
#define FS_TYPE_FREE 2 /* tombstoned directory slot, free for reuse */

#define FS_ROOT      ((int32_t)-1)
#define FS_NOT_FOUND ((int32_t)-2)

/* Negative return codes shared by the mutating calls below. */
#define FS_OK            0
#define FS_ERR_EXISTS   -1
#define FS_ERR_NOTDIR   -2
#define FS_ERR_ISDIR    -3
#define FS_ERR_FULL     -4
#define FS_ERR_NAME     -5
#define FS_ERR_NOSPACE  -6
#define FS_ERR_NOTEMPTY -7
#define FS_ERR_CYCLE    -8

#define FS_WRITE_OVERWRITE 0
#define FS_WRITE_APPEND    1

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

/* --- mutation API ---
 * All of these persist the directory (and superblock, when allocation
 * state changed) to disk before returning. `name` may not contain '/' or
 * ' ', may not be "." or "..", and must fit in FS_NAME_LEN - 1 bytes. */

/* Creates an empty file/directory under `parent`. Returns the new entry's
 * index (>= 0), or a negative FS_ERR_* code. */
int32_t fs_create(int32_t parent, const char *name);
int32_t fs_mkdir(int32_t parent, const char *name);

/* Overwrites or appends to a file's contents, allocating storage as needed
 * and freeing any storage it no longer needs. Returns FS_OK or FS_ERR_*. */
int fs_write_file(int32_t index, const void *data, uint32_t size, int mode);

/* Removes a file, or a directory that has no remaining children. Returns
 * FS_OK or FS_ERR_*. */
int fs_delete(int32_t index);

/* Renames and/or moves an entry to `new_parent`. Returns FS_OK or FS_ERR_*. */
int fs_rename(int32_t index, int32_t new_parent, const char *new_name);

#endif
