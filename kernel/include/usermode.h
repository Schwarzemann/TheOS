#ifndef THEOS_USERMODE_H
#define THEOS_USERMODE_H

#include <stdint.h>

__attribute__((noreturn)) void enter_usermode(uint32_t entry, uint32_t user_stack);

void user_demo_spawn(void);

/* Ring3 entry point of the demo program in kernel/user/demo.c. */
void user_demo_main(void);

#endif
