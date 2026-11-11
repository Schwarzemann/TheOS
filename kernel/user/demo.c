/* Ring3 demo program. Everything in kernel/user/ is linked into the image's
 * user region (see linker.ld), the only part ring3 can read. It must not call
 * kernel functions, including ones the compiler may insert such as memcpy:
 * those live in kernel-only pages and fault. */

#include <usermode.h>
#include <syscall.h>

static inline int sys_write(const char *s) {
    int result;
    __asm__ volatile ("int $0x80" : "=a"(result) : "a"(SYS_WRITE), "b"(s) : "memory");
    return result;
}

static inline void sys_exit(void) {
    __asm__ volatile ("int $0x80" : : "a"(SYS_EXIT) : "memory");
}

void user_demo_main(void) {
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
