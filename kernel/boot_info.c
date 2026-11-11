#include <boot_info.h>

#define BOOT_INFO_MEM_KB  ((volatile const uint32_t *)0x0500)
#define BOOT_INFO_DISK_KB ((volatile const uint32_t *)0x0504)

uint32_t boot_mem_kb;
uint32_t boot_disk_kb;

void boot_info_save(void) {
    boot_mem_kb = *BOOT_INFO_MEM_KB;
    boot_disk_kb = *BOOT_INFO_DISK_KB;
}
