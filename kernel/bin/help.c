#include <cmds.h>
#include <vga.h>

void cmd_help(const char *arg) {
    (void)arg;
    vga_print("Commands: help, memory, disk, uptime, meminfo, ls, cat <file>, cd <dir>, pwd, clear,\n");
    vga_print("touch <name>, mkdir <name>, rm <file>, rmdir <dir>, write <file> <text>,\n");
    vga_print("append <file> <text>, mv <src> <dest>, cp <src> <dest>\n");
}
