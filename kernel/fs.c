#include <fs.h>
#include <ata.h>
#include <string.h>
#include <serial.h>
#include <panic.h>
#include <heap.h>

#define FS_DIR_SECTORS 5
#define FS_RECORD_SIZE 40

/* Superblock layout (sector 0), written by fs_sync_superblock and by
 * scripts/mkfs_tofs.py:
 *   0  : magic "TOFS"
 *   4  : uint32 entry_count
 *   8  : uint32 next_free_lba   (bump pointer for space never yet used)
 *   12 : uint32 free_count      (number of valid free extents below)
 *   16 : FS_MAX_FREE_EXTENTS * {uint32 start_lba, uint32 count} */
#define FS_SB_FREE_LIST_OFFSET 16

typedef struct {
    uint32_t start_lba;
    uint32_t count;
} free_extent_t;

static fs_entry_t entries[FS_MAX_ENTRIES];
static uint32_t entry_count;

static uint32_t next_free_lba;
static free_extent_t free_extents[FS_MAX_FREE_EXTENTS];
static uint32_t free_count;

static void fs_sync_superblock(void) {
    uint8_t sb[512];
    memset(sb, 0, sizeof(sb));
    memcpy(sb, "TOFS", 4);
    memcpy(sb + 4, &entry_count, sizeof(uint32_t));
    memcpy(sb + 8, &next_free_lba, sizeof(uint32_t));
    memcpy(sb + 12, &free_count, sizeof(uint32_t));
    for (uint32_t i = 0; i < free_count; i++) {
        uint8_t *rec = sb + FS_SB_FREE_LIST_OFFSET + i * 8;
        memcpy(rec, &free_extents[i].start_lba, sizeof(uint32_t));
        memcpy(rec + 4, &free_extents[i].count, sizeof(uint32_t));
    }
    ata_write(FS_BASE_LBA, 1, sb);
}

static void fs_sync_directory(void) {
    uint8_t dir[FS_DIR_SECTORS * 512];
    memset(dir, 0, sizeof(dir));
    for (uint32_t i = 0; i < entry_count; i++) {
        uint8_t *rec = dir + i * FS_RECORD_SIZE;
        memcpy(rec, entries[i].name, FS_NAME_LEN);
        memcpy(rec + FS_NAME_LEN, &entries[i].type, sizeof(uint32_t));
        memcpy(rec + FS_NAME_LEN + 4, &entries[i].parent, sizeof(int32_t));
        memcpy(rec + FS_NAME_LEN + 8, &entries[i].start_lba, sizeof(uint32_t));
        memcpy(rec + FS_NAME_LEN + 12, &entries[i].size_bytes, sizeof(uint32_t));
    }
    ata_write(FS_BASE_LBA + 1, FS_DIR_SECTORS, dir);
}

/* First-fit allocator: reuses a freed extent if one is big enough,
 * otherwise falls back to bumping next_free_lba. Returns 0 on failure
 * (0 is never a valid data LBA: sector 0 is the boot sector). */
static uint32_t fs_alloc_sectors(uint32_t want) {
    if (want == 0) {
        return 0;
    }
    for (uint32_t i = 0; i < free_count; i++) {
        if (free_extents[i].count >= want) {
            uint32_t lba = free_extents[i].start_lba;
            if (free_extents[i].count == want) {
                for (uint32_t j = i; j < free_count - 1; j++) {
                    free_extents[j] = free_extents[j + 1];
                }
                free_count--;
            } else {
                free_extents[i].start_lba += want;
                free_extents[i].count -= want;
            }
            return lba;
        }
    }

    uint32_t region_end = FS_BASE_LBA + FS_TOTAL_SECTORS;
    if (next_free_lba + want > region_end) {
        return 0;
    }
    uint32_t lba = next_free_lba;
    next_free_lba += want;
    return lba;
}

/* Returns an extent to the free list, merging it with an adjacent extent
 * when possible. If the free list is full, the space is leaked. */
static void fs_free_sectors(uint32_t start_lba, uint32_t count) {
    if (count == 0) {
        return;
    }
    for (uint32_t i = 0; i < free_count; i++) {
        if (free_extents[i].start_lba + free_extents[i].count == start_lba) {
            free_extents[i].count += count;
            return;
        }
        if (start_lba + count == free_extents[i].start_lba) {
            free_extents[i].start_lba = start_lba;
            free_extents[i].count += count;
            return;
        }
    }
    if (free_count < FS_MAX_FREE_EXTENTS) {
        free_extents[free_count].start_lba = start_lba;
        free_extents[free_count].count = count;
        free_count++;
    }
}

static int fs_valid_name(const char *name) {
    size_t len = strlen(name);
    if (len == 0 || len >= FS_NAME_LEN) {
        return 0;
    }
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        if (name[i] == '/' || name[i] == ' ') {
            return 0;
        }
    }
    return 1;
}

void fs_init(void) {
    uint8_t superblock[512];
    ata_read(FS_BASE_LBA, 1, superblock);

    if (memcmp(superblock, "TOFS", 4) != 0) {
        panic("fs_init: TOFS superblock magic mismatch");
    }
    memcpy(&entry_count, superblock + 4, sizeof(uint32_t));
    if (entry_count > FS_MAX_ENTRIES) {
        panic("fs_init: entry count exceeds FS_MAX_ENTRIES");
    }
    memcpy(&next_free_lba, superblock + 8, sizeof(uint32_t));
    memcpy(&free_count, superblock + 12, sizeof(uint32_t));
    if (free_count > FS_MAX_FREE_EXTENTS) {
        panic("fs_init: free extent count exceeds FS_MAX_FREE_EXTENTS");
    }
    for (uint32_t i = 0; i < free_count; i++) {
        uint8_t *rec = superblock + FS_SB_FREE_LIST_OFFSET + i * 8;
        memcpy(&free_extents[i].start_lba, rec, sizeof(uint32_t));
        memcpy(&free_extents[i].count, rec + 4, sizeof(uint32_t));
    }

    uint8_t dir[FS_DIR_SECTORS * 512];
    ata_read(FS_BASE_LBA + 1, FS_DIR_SECTORS, dir);

    for (uint32_t i = 0; i < entry_count; i++) {
        uint8_t *rec = dir + i * FS_RECORD_SIZE;
        memcpy(entries[i].name, rec, FS_NAME_LEN);
        entries[i].name[FS_NAME_LEN - 1] = '\0';
        memcpy(&entries[i].type, rec + FS_NAME_LEN, sizeof(uint32_t));
        memcpy(&entries[i].parent, rec + FS_NAME_LEN + 4, sizeof(int32_t));
        memcpy(&entries[i].start_lba, rec + FS_NAME_LEN + 8, sizeof(uint32_t));
        memcpy(&entries[i].size_bytes, rec + FS_NAME_LEN + 12, sizeof(uint32_t));
    }

    serial_write("TheOS: TOFS mounted, ");
    serial_uint(entry_count);
    serial_write(" entries\n");
}

uint32_t fs_entry_count(void) {
    return entry_count;
}

const fs_entry_t *fs_entry_at(uint32_t index) {
    if (index >= entry_count) {
        return 0;
    }
    return &entries[index];
}

int fs_is_dir(int32_t index) {
    if (index == FS_ROOT) {
        return 1;
    }
    if (index < 0 || (uint32_t)index >= entry_count) {
        return 0;
    }
    return entries[index].type == FS_TYPE_DIR;
}

int32_t fs_find_child(int32_t dir, const char *name) {
    for (uint32_t i = 0; i < entry_count; i++) {
        if (entries[i].type != FS_TYPE_FREE && entries[i].parent == dir &&
            strcmp(entries[i].name, name) == 0) {
            return (int32_t)i;
        }
    }
    return FS_NOT_FOUND;
}

int fs_resolve(int32_t from, const char *path, int32_t *out_index) {
    int32_t cur = (path[0] == '/') ? FS_ROOT : from;
    const char *p = (path[0] == '/') ? path + 1 : path;

    while (*p != '\0') {
        char comp[FS_NAME_LEN];
        size_t i = 0;
        while (p[i] != '\0' && p[i] != '/' && i < FS_NAME_LEN - 1) {
            comp[i] = p[i];
            i++;
        }
        comp[i] = '\0';
        p += i;
        if (*p == '/') {
            p++;
        }

        if (comp[0] == '\0' || strcmp(comp, ".") == 0) {
            continue;
        }
        if (strcmp(comp, "..") == 0) {
            if (cur != FS_ROOT) {
                cur = entries[cur].parent;
            }
            continue;
        }
        if (!fs_is_dir(cur)) {
            return 0;
        }
        int32_t child = fs_find_child(cur, comp);
        if (child == FS_NOT_FOUND) {
            return 0;
        }
        cur = child;
    }

    *out_index = cur;
    return 1;
}

uint32_t fs_sectors(const fs_entry_t *entry) {
    return (entry->size_bytes + 511) / 512;
}

uint32_t fs_read(const fs_entry_t *entry, void *buf) {
    ata_read(entry->start_lba, fs_sectors(entry), buf);
    return entry->size_bytes;
}

static int32_t fs_new_entry(int32_t parent, const char *name, uint32_t type) {
    if (!fs_is_dir(parent)) {
        return FS_ERR_NOTDIR;
    }
    if (!fs_valid_name(name)) {
        return FS_ERR_NAME;
    }
    if (fs_find_child(parent, name) != FS_NOT_FOUND) {
        return FS_ERR_EXISTS;
    }

    int32_t idx = -1;
    for (uint32_t i = 0; i < entry_count; i++) {
        if (entries[i].type == FS_TYPE_FREE) {
            idx = (int32_t)i;
            break;
        }
    }
    if (idx < 0) {
        if (entry_count >= FS_MAX_ENTRIES) {
            return FS_ERR_FULL;
        }
        idx = (int32_t)entry_count;
        entry_count++;
    }

    memset(entries[idx].name, 0, FS_NAME_LEN);
    memcpy(entries[idx].name, name, strlen(name));
    entries[idx].type = type;
    entries[idx].parent = parent;
    entries[idx].start_lba = 0;
    entries[idx].size_bytes = 0;

    fs_sync_directory();
    fs_sync_superblock();
    return idx;
}

int32_t fs_create(int32_t parent, const char *name) {
    return fs_new_entry(parent, name, FS_TYPE_FILE);
}

int32_t fs_mkdir(int32_t parent, const char *name) {
    return fs_new_entry(parent, name, FS_TYPE_DIR);
}

int fs_write_file(int32_t index, const void *data, uint32_t size, int mode) {
    if (index < 0 || (uint32_t)index >= entry_count || entries[index].type == FS_TYPE_FREE) {
        return FS_ERR_NAME;
    }
    fs_entry_t *e = &entries[index];
    if (e->type != FS_TYPE_FILE) {
        return FS_ERR_ISDIR;
    }

    uint32_t old_sectors = fs_sectors(e);
    uint32_t old_start = e->start_lba;
    uint32_t old_size = (mode == FS_WRITE_APPEND) ? e->size_bytes : 0;
    uint32_t new_size = old_size + size;
    uint32_t new_sectors = (new_size + 511) / 512;

    uint8_t *buf = 0;
    if (new_sectors > 0) {
        buf = kmalloc(new_sectors * 512);
        if (buf == 0) {
            return FS_ERR_NOSPACE;
        }
        memset(buf, 0, new_sectors * 512);
        if (old_size > 0) {
            fs_read(e, buf);
        }
        memcpy(buf + old_size, data, size);
    }

    uint32_t new_lba = 0;
    if (new_sectors > 0) {
        new_lba = fs_alloc_sectors(new_sectors);
        if (new_lba == 0) {
            kfree(buf);
            return FS_ERR_NOSPACE;
        }
    }

    if (old_sectors > 0) {
        fs_free_sectors(old_start, old_sectors);
    }

    if (new_sectors > 0) {
        ata_write(new_lba, new_sectors, buf);
        kfree(buf);
    }

    e->start_lba = new_sectors > 0 ? new_lba : 0;
    e->size_bytes = new_size;

    fs_sync_directory();
    fs_sync_superblock();
    return FS_OK;
}

int fs_delete(int32_t index) {
    if (index == FS_ROOT || index < 0 || (uint32_t)index >= entry_count) {
        return FS_ERR_NAME;
    }
    fs_entry_t *e = &entries[index];
    if (e->type == FS_TYPE_FREE) {
        return FS_ERR_NAME;
    }

    if (e->type == FS_TYPE_DIR) {
        for (uint32_t i = 0; i < entry_count; i++) {
            if (entries[i].type != FS_TYPE_FREE && entries[i].parent == index) {
                return FS_ERR_NOTEMPTY;
            }
        }
    } else {
        uint32_t sectors = fs_sectors(e);
        if (sectors > 0) {
            fs_free_sectors(e->start_lba, sectors);
        }
    }

    memset(e->name, 0, FS_NAME_LEN);
    e->type = FS_TYPE_FREE;
    e->parent = FS_ROOT;
    e->start_lba = 0;
    e->size_bytes = 0;

    fs_sync_directory();
    fs_sync_superblock();
    return FS_OK;
}

int fs_rename(int32_t index, int32_t new_parent, const char *new_name) {
    if (index < 0 || (uint32_t)index >= entry_count || entries[index].type == FS_TYPE_FREE) {
        return FS_ERR_NAME;
    }
    if (!fs_is_dir(new_parent)) {
        return FS_ERR_NOTDIR;
    }
    if (!fs_valid_name(new_name)) {
        return FS_ERR_NAME;
    }

    if (entries[index].type == FS_TYPE_DIR) {
        int32_t p = new_parent;
        while (p != FS_ROOT) {
            if (p == index) {
                return FS_ERR_CYCLE;
            }
            p = entries[p].parent;
        }
    }

    int32_t existing = fs_find_child(new_parent, new_name);
    if (existing != FS_NOT_FOUND && existing != index) {
        return FS_ERR_EXISTS;
    }

    memset(entries[index].name, 0, FS_NAME_LEN);
    memcpy(entries[index].name, new_name, strlen(new_name));
    entries[index].parent = new_parent;

    fs_sync_directory();
    return FS_OK;
}
