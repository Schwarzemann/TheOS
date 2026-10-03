#include <cmds.h>
#include <vga.h>
#include <boot_info.h>

void cmd_disk(const char *arg) {
    (void)arg;
    vga_print_uint(BOOT_DISK_KB);
    vga_print(" KB disk size\n");
}
