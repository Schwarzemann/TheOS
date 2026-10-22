#include <cmds.h>
#include <vga.h>
#include <fs.h>
#include <cwd.h>
#include <bin.h>

void cmd_rm(const char *arg) {
    if (arg[0] == '\0') {
        vga_print("usage: rm <file>\n");
        return;
    }

    int32_t index;
    if (!fs_resolve(cwd_get(), arg, &index)) {
        vga_print("rm: no such file: ");
        vga_print(arg);
        vga_putc('\n');
        return;
    }
    if (fs_is_dir(index)) {
        vga_print("rm: is a directory (use rmdir): ");
        vga_print(arg);
        vga_putc('\n');
        return;
    }

    int err = fs_delete(index);
    if (err < 0) {
        print_fs_error("rm", err);
    }
}
