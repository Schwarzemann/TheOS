#ifndef THEOS_GDT_H
#define THEOS_GDT_H

#include <stdint.h>

#define GDT_KCODE 0x08
#define GDT_KDATA 0x10
#define GDT_UCODE   (0x18 | 3)
#define GDT_UDATA   (0x20 | 3)
#define GDT_TSS         0x28

void gdt_init(void);

/* Sets TSS.esp0 so ring3->ring0 interrupts and syscalls land on this task's kernel stack.
 * Called on every task switch. */
void gdt_set_kstack(uint32_t esp0);

#endif
