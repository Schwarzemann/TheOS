#include <cmds.h>
#include <vga.h>

void cmd_clear(const char *arg) {
    (void)arg;
    vga_clear();
}
