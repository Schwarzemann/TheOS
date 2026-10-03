#ifndef THEOS_PIC_H
#define THEOS_PIC_H

#define PIC1_OFFSET 0x20   /* IRQ0-7  -> ISR 32-39 */
#define PIC2_OFFSET 0x28   /* IRQ8-15 -> ISR 40-47 */

void pic_remap(void);
void pic_eoi(int irq);
void pic_mask(int irq);
void pic_unmask(int irq);

#endif
