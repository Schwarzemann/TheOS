#include <cmds.h>
#include <vga.h>
#include <fs.h>
#include <cwd.h>
#include <bin.h>

void cmd_mkdir(const char *arg) {
    if (arg[0] == '\0') {
        vga_print("usage: mkdir <path>\n");
        return;
    }

    int32_t parent;
    char name[FS_NAME_LEN];
    int32_t result = fs_split_path(cwd_get(), arg, &parent, name);
    if (result == FS_OK) {
        result = fs_mkdir(parent, name);
    }
    if (result < 0) {
        print_fs_error("mkdir", result);
    }
}
