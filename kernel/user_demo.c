/* Ring3 demo: proves privilege separation by running unprivileged code
 * that talks to the kernel only through int 0x80 syscalls. */

#include <usermode.h>
#include <task.h>
#include <heap.h>
#include <syscall.h>
#include <panic.h>

#define USER_STACK_SZ 8192

static inline void sys_write(const char *s) {
    __asm__ volatile ("int $0x80" : : "a"(SYS_WRITE), "b"(s) : "memory");
}

static inline void sys_exit(void) {
    __asm__ volatile ("int $0x80" : : "a"(SYS_EXIT) : "memory");
}

static void ring3_entry(void) {
    for (int i = 0; i < 3; i++) {
        sys_write("[ring3] user-mode demo task is running as expected\n");
        /* Busy-wait so the PIT can preempt this task between writes. */
        for (volatile int spin = 0; spin < 3000000; spin++) {
        }
    }
    sys_exit();
    for (;;) {
    } /* unreachable: a dead task is never scheduled again */
}

/* Kernel task that drops to ring3 on its own user stack. */
static void user_demo_entry(void) {
    uint8_t *stack = kmalloc(USER_STACK_SZ);
    if (stack == 0) {
        panic("user_demo: out of memory for the user stack");
    }
    enter_usermode((uint32_t)ring3_entry, (uint32_t)(stack + USER_STACK_SZ));
}

void user_demo_spawn(void) {
    task_create("user_demo", user_demo_entry);
}
