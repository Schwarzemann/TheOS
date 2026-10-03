#ifndef THEOS_ISR_H
#define THEOS_ISR_H

#include <stdint.h>

typedef struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

typedef void (*isr_handler_t)(registers_t *regs);

void isr_init(void);

/* Registers a handler for IRQ `irq` (0-15) and unmasks it in the PIC. */
void irq_register(int irq, isr_handler_t handler);

/* Registers a handler for interrupt vector `n` (e.g. 0x80 for syscalls). Unhandled
 * CPU exceptions (0-31) dump registers and panic. */
void isr_register(int n, isr_handler_t handler);

#endif
