#include <cmds.h>
#include <vga.h>
#include <pmm.h>
#include <heap.h>

void cmd_meminfo(const char *arg) {
    (void)arg;
    vga_print_uint(pmm_total() * PMM_FRAME_SIZE / 1024);
    vga_print(" KB total, ");
    vga_print_uint(pmm_free_count() * PMM_FRAME_SIZE / 1024);
    vga_print(" KB free (physical)\n");
    vga_print_uint((unsigned int)(heap_free_bytes() / 1024));
    vga_print(" KB free on kernel heap\n");
}
