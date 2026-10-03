#include <cmds.h>
#include <vga.h>
#include <cwd.h>

void cmd_cd(const char *arg) {
    const char *path = (arg[0] == '\0') ? "/" : arg;
    if (!cwd_chdir(path)) {
        vga_print("cd: no such directory: ");
        vga_print(path);
        vga_putc('\n');
    }
}
