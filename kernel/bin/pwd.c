#include <cmds.h>
#include <vga.h>
#include <cwd.h>

#define PWD_BUF_SIZE 128

void cmd_pwd(const char *arg) {
    (void)arg;
    char path[PWD_BUF_SIZE];
    cwd_path(path, sizeof(path));
    vga_print(path);
    vga_putc('\n');
}
