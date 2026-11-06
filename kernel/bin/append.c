#include <cmds.h>
#include <vga.h>
#include <fs.h>
#include <bin.h>
#include <string.h>

void cmd_append(const char *arg) {
    size_t i = 0;
    while (arg[i] != '\0' && arg[i] != ' ') {
        i++;
    }
    char path[64]; /* shell command lines are capped at 64 bytes (CMD_BUF_SIZE) */
    if (i == 0 || arg[i] != ' ' || i >= sizeof(path)) {
        vga_print("usage: append <file> <text>\n");
        return;
    }

    memcpy(path, arg, i);
    path[i] = '\0';
    const char *text = arg + i + 1;

    int32_t index = file_for_write("append", path);
    if (index < 0) {
        return;
    }

    int err = fs_write_file(index, text, (uint32_t)strlen(text), FS_WRITE_APPEND);
    if (err < 0) {
        print_fs_error("append", err);
    }
}
