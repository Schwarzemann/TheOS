#ifndef THEOS_PIC_H
#define THEOS_PIC_H

#include <stdint.h>

#define PIC1_OFFSET 0x20   /* IRQ0-7  -> ISR 32-39 */
#define PIC2_OFFSET 0x28   /* IRQ8-15 -> ISR 40-47 */

void pic_remap(void);
void pic_eoi(int irq);
void pic_mask(int irq);
void pic_unmask(int irq);

/* In-service register of both PICs: bit n is set while IRQ n is being handled. */
uint16_t pic_read_isr(void);

#endif
