#ifndef THEOS_PAGING_H
#define THEOS_PAGING_H

#include <stdint.h>

#define PAGE_SIZE    4096
#define PAGE_PRESENT 0x1
#define PAGE_WRITE   0x2
#define PAGE_USER    0x4

/* Ring3 stacks are mapped from here up, above the identity map of RAM. */
#define USER_STACKS_BASE 0xB0000000u

/* Identity-maps all RAM except page 0, kernel-only, then enables paging with
 * write protection enforced in ring0 too. Only the image's ring3 region (see
 * linker.ld) is user-accessible. Needs pmm_init() and isr_init() first.
 * No demand paging: every mapped page is backed by a frame up front. */
void paging_init(void);

/* Maps the page at `virt` to the frame at `phys` with PAGE_* `flags` (PRESENT is
 * implied), replacing any existing mapping. Returns 0, or -1 if a page table
 * couldn't be allocated. Addresses are rounded down to a page boundary. */
int paging_map(uint32_t virt, uint32_t phys, uint32_t flags);

/* Removes the mapping for the page at `virt`, if there is one. */
void paging_unmap(uint32_t virt);

/* The page table entry for `virt` (frame address | PAGE_* flags), or 0 if the
 * page isn't mapped. */
uint32_t paging_lookup(uint32_t virt);

/* True if all of [addr, addr + len) is mapped with PAGE_USER, and with
 * PAGE_WRITE too when `write` is set. */
int paging_user_ok(uint32_t addr, uint32_t len, int write);

#endif
