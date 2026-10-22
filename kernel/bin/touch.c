#include <cmds.h>
#include <vga.h>
#include <fs.h>
#include <cwd.h>
#include <bin.h>

void cmd_touch(const char *arg) {
    if (arg[0] == '\0') {
        vga_print("usage: touch <name>\n");
        return;
    }
    int32_t result = fs_create(cwd_get(), arg);
    if (result < 0) {
        print_fs_error("touch", result);
    }
}
