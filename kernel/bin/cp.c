#include <cmds.h>
#include <vga.h>
#include <fs.h>
#include <cwd.h>
#include <bin.h>
#include <string.h>
#include <heap.h>

void cmd_cp(const char *arg) {
    size_t i = 0;
    while (arg[i] != '\0' && arg[i] != ' ') {
        i++;
    }
    if (i == 0 || arg[i] != ' ') {
        vga_print("usage: cp <src> <dest>\n");
        return;
    }
    char src[64]; /* shell command lines are capped at 64 bytes (CMD_BUF_SIZE) */
    if (i >= sizeof(src)) {
        vga_print("cp: name too long\n");
        return;
    }
    memcpy(src, arg, i);
    src[i] = '\0';
    const char *dest = arg + i + 1;
    if (dest[0] == '\0') {
        vga_print("usage: cp <src> <dest>\n");
        return;
    }

    int32_t src_index;
    if (!fs_resolve(cwd_get(), src, &src_index)) {
        vga_print("cp: no such file: ");
        vga_print(src);
        vga_putc('\n');
        return;
    }
    if (fs_is_dir(src_index)) {
        vga_print("cp: cannot copy a directory\n");
        return;
    }

    int32_t dest_parent;
    const char *dest_name;
    char keep_name[FS_NAME_LEN];
    int32_t maybe_dir;
    if (fs_resolve(cwd_get(), dest, &maybe_dir) && fs_is_dir(maybe_dir)) {
        dest_parent = maybe_dir;
        strncpy(keep_name, fs_entry_at((uint32_t)src_index)->name, FS_NAME_LEN);
        dest_name = keep_name;
    } else {
        dest_parent = cwd_get();
        dest_name = dest;
    }

    const fs_entry_t *src_entry = fs_entry_at((uint32_t)src_index);
    uint32_t size = src_entry->size_bytes;
    uint8_t *buf = 0;
    if (size > 0) {
        buf = kmalloc(fs_sectors(src_entry) * 512);
        if (buf == 0) {
            vga_print("cp: out of memory\n");
            return;
        }
        fs_read(src_entry, buf);
    }

    int32_t dest_index = fs_find_child(dest_parent, dest_name);
    if (dest_index == FS_NOT_FOUND) {
        dest_index = fs_create(dest_parent, dest_name);
        if (dest_index < 0) {
            if (buf != 0) {
                kfree(buf);
            }
            print_fs_error("cp", dest_index);
            return;
        }
    } else if (fs_is_dir(dest_index)) {
        if (buf != 0) {
            kfree(buf);
        }
        vga_print("cp: is a directory\n");
        return;
    }

    int err = fs_write_file(dest_index, buf, size, FS_WRITE_OVERWRITE);
    if (buf != 0) {
        kfree(buf);
    }
    if (err < 0) {
        print_fs_error("cp", err);
    }
}
