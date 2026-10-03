#include <cmds.h>
#include <vga.h>

void cmd_help(const char *arg) {
    (void)arg;
    vga_print("Commands: help, memory, disk, uptime, meminfo, ls, cat <file>, cd <dir>, pwd, clear\n");
}
