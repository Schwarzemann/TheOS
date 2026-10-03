#ifndef THEOS_PANIC_H
#define THEOS_PANIC_H

/* Prints msg to VGA and serial, then halts forever. */
__attribute__((noreturn)) void panic(const char *msg);

#endif
