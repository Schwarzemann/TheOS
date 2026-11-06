#include <cmds.h>
#include <vga.h>
#include <fs.h>
#include <cwd.h>
#include <bin.h>
#include <string.h>

void cmd_mv(const char *arg) {
    size_t i = 0;
    while (arg[i] != '\0' && arg[i] != ' ') {
        i++;
    }
    if (i == 0 || arg[i] != ' ') {
        vga_print("usage: mv <src> <dest>\n");
        return;
    }
    char src[64]; /* shell command lines are capped at 64 bytes (CMD_BUF_SIZE) */
    if (i >= sizeof(src)) {
        vga_print("mv: name too long\n");
        return;
    }
    memcpy(src, arg, i);
    src[i] = '\0';
    const char *dest = arg + i + 1;
    if (dest[0] == '\0') {
        vga_print("usage: mv <src> <dest>\n");
        return;
    }

    int32_t src_index;
    if (!fs_resolve(cwd_get(), src, &src_index)) {
        vga_print("mv: no such file: ");
        vga_print(src);
        vga_putc('\n');
        return;
    }

    if (src_index == FS_ROOT) {
        vga_print("mv: cannot move the root directory\n");
        return;
    }

    int32_t new_parent;
    char new_name[FS_NAME_LEN];
    int32_t maybe_dir;
    if (fs_resolve(cwd_get(), dest, &maybe_dir) && fs_is_dir(maybe_dir)) {
        new_parent = maybe_dir;
        strncpy(new_name, fs_entry_at((uint32_t)src_index)->name, FS_NAME_LEN);
    } else {
        int err = fs_split_path(cwd_get(), dest, &new_parent, new_name);
        if (err < 0) {
            print_fs_error("mv", err);
            return;
        }
    }

    int err = fs_rename(src_index, new_parent, new_name);
    if (err < 0) {
        print_fs_error("mv", err);
    }
}
