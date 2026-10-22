#include <cmds.h>
#include <vga.h>
#include <fs.h>
#include <cwd.h>
#include <bin.h>

void cmd_mkdir(const char *arg) {
    if (arg[0] == '\0') {
        vga_print("usage: mkdir <name>\n");
        return;
    }
    int32_t result = fs_mkdir(cwd_get(), arg);
    if (result < 0) {
        print_fs_error("mkdir", result);
    }
}
