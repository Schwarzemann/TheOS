#include <cmds.h>
#include <vga.h>
#include <fs.h>
#include <cwd.h>
#include <bin.h>
#include <string.h>

void cmd_write(const char *arg) {
    size_t i = 0;
    while (arg[i] != '\0' && arg[i] != ' ') {
        i++;
    }
    if (i == 0 || arg[i] != ' ' || i >= FS_NAME_LEN) {
        vga_print("usage: write <file> <text>\n");
        return;
    }

    char name[FS_NAME_LEN];
    memcpy(name, arg, i);
    name[i] = '\0';
    const char *text = arg + i + 1;

    int32_t index = fs_find_child(cwd_get(), name);
    if (index == FS_NOT_FOUND) {
        index = fs_create(cwd_get(), name);
        if (index < 0) {
            print_fs_error("write", index);
            return;
        }
    } else if (fs_is_dir(index)) {
        vga_print("write: is a directory: ");
        vga_print(name);
        vga_putc('\n');
        return;
    }

    int err = fs_write_file(index, text, (uint32_t)strlen(text), FS_WRITE_OVERWRITE);
    if (err < 0) {
        print_fs_error("write", err);
    }
}
