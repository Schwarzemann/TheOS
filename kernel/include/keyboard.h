#ifndef THEOS_KEYBOARD_H
#define THEOS_KEYBOARD_H

void kbd_init(void);

/* Blocks the calling task until a key is available, then returns its ASCII
 * value. Needs the scheduler running. */
char kbd_getc(void);

#endif
