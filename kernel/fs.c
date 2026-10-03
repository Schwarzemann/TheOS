#include <fs.h>
#include <ata.h>
#include <string.h>
#include <serial.h>
#include <panic.h>

#define FS_DIR_SECTORS 5
#define FS_RECORD_SIZE 40

static fs_entry_t entries[FS_MAX_ENTRIES];
static uint32_t entry_count;

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
        if (entries[i].parent == dir && strcmp(entries[i].name, name) == 0) {
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
