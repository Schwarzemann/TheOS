#ifndef THEOS_KEYBOARD_H
#define THEOS_KEYBOARD_H

void kbd_init(void);

/* Blocks (halting the CPU between interrupts) until a key is available,
 * then returns its ASCII value. */
char kbd_getc(void);

#endif
