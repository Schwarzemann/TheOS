/* Ring3 demo: proves privilege separation by running unprivileged code
 * (kernel/user/demo.c) that talks to the kernel only through int 0x80 syscalls. */

#include <usermode.h>
#include <task.h>
#include <serial.h>

#define USER_STACK_PAGES 2

/* Kernel task that drops to ring3 on its own user stack. */
static void user_demo_entry(void) {
    uint32_t stack_top = task_map_user_stack(USER_STACK_PAGES);
    if (stack_top == 0) {
        serial_write("user_demo: out of memory for the user stack\n");
        return;
    }
    enter_usermode((uint32_t)user_demo_main, stack_top);
}

void user_demo_spawn(void) {
    task_create("user_demo", user_demo_entry);
}
