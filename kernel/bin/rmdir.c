#include <cmds.h>
#include <vga.h>
#include <fs.h>
#include <cwd.h>
#include <bin.h>

void cmd_rmdir(const char *arg) {
    if (arg[0] == '\0') {
        vga_print("usage: rmdir <dir>\n");
        return;
    }

    int32_t index;
    if (!fs_resolve(cwd_get(), arg, &index)) {
        vga_print("rmdir: no such directory: ");
        vga_print(arg);
        vga_putc('\n');
        return;
    }
    if (!fs_is_dir(index)) {
        vga_print("rmdir: not a directory: ");
        vga_print(arg);
        vga_putc('\n');
        return;
    }

    int err = fs_delete(index);
    if (err < 0) {
        print_fs_error("rmdir", err);
    }
}
