#ifndef THEOS_BOOT_INFO_H
#define THEOS_BOOT_INFO_H

#include <stdint.h>

/* What TheBOOT detected: RAM and boot disk size in KiB (0 if detection failed). */
extern uint32_t boot_mem_kb;
extern uint32_t boot_disk_kb;

/* Copies the values TheBOOT left at physical 0x500. Must run before
 * paging_init(), which unmaps that page. */
void boot_info_save(void);

#endif
