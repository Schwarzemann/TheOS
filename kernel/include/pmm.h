#ifndef THEOS_PMM_H
#define THEOS_PMM_H

#include <stdint.h>

#define PMM_FRAME_SIZE 4096

/* mem_kb: total RAM in KiB, as detected by TheBOOT (BOOT_MEM_KB).
 * Reserves frame 0 through the end of the kernel image automatically. */
void pmm_init(uint32_t mem_kb);

/* Returns the physical address of a free 4 KiB frame, or 0 if out of
 * memory. */
uint32_t pmm_alloc(void);
void pmm_free(uint32_t phys_addr);

uint32_t pmm_total(void);
uint32_t pmm_free_count(void);

#endif
