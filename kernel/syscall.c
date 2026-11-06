#include <syscall.h>
#include <isr.h>
#include <vga.h>
#include <task.h>

static void sys_dispatch(registers_t *regs) {
    /* The interrupt gate turned these off, but a syscall runs on this task's own
     * kernel stack and can take a long time, so it stays preemptible. */
    __asm__ volatile ("sti");

    switch (regs->eax) {
        case SYS_WRITE:
            vga_print((const char *)regs->ebx); /* vga_putc mirrors to serial too */
            break;
        case SYS_EXIT:
            task_exit(); /* never returns */
            break;
        default:
            break;
    }
}

void sys_init(void) {
    isr_register(128, sys_dispatch);
}
