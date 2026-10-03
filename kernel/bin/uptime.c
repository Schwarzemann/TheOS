#include <cmds.h>
#include <vga.h>
#include <pit.h>

void cmd_uptime(const char *arg) {
    (void)arg;
    vga_print_uint(pit_ticks() / PIT_HZ);
    vga_print(" seconds since boot\n");
}
