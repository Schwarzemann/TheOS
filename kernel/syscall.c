#include <syscall.h>
#include <isr.h>
#include <vga.h>
#include <task.h>

static void sys_dispatch(registers_t *regs) {
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
