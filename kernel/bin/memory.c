#include <cmds.h>
#include <vga.h>
#include <boot_info.h>

void cmd_memory(const char *arg) {
    (void)arg;
    vga_print_uint(boot_mem_kb);
    vga_print(" KB RAM detected\n");
}
