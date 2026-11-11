#include <syscall.h>
#include <isr.h>
#include <vga.h>
#include <task.h>
#include <paging.h>
#include <string.h>
#include <sync.h>

/* Copies the NUL-terminated user string at `addr` into buf (max + 1 bytes).
 * Returns its length or a SYS_E* code. Interrupts stay off while copying, so
 * no other task can unmap the pages in between checking and reading them. */
static int copy_user_string(uint32_t addr, char *buf, uint32_t max) {
    int result = SYS_EINVAL;
    uint32_t flags = irq_save();
    for (uint32_t i = 0; i <= max; i++) {
        uint32_t a = addr + i;
        if (a < addr || ((i == 0 || (a & (PAGE_SIZE - 1)) == 0) && !paging_user_ok(a, 1, 0))) {
            result = SYS_EFAULT;
            break;
        }
        buf[i] = *(const char *)a;
        if (buf[i] == '\0') {
            result = (int)i;
            break;
        }
    }
    irq_restore(flags);
    return result;
}

static int sys_write(uint32_t addr) {
    char buf[SYS_WRITE_MAX + 1];
    int len = copy_user_string(addr, buf, SYS_WRITE_MAX);
    if (len > 0) {
        vga_print_n(buf, (size_t)len); /* vga_putc mirrors to serial too */
    }
    return len;
}

static void sys_dispatch(registers_t *regs) {
    /* The interrupt gate turned these off, but a syscall runs on this task's own
     * kernel stack and can take a long time, so it stays preemptible. */
    __asm__ volatile ("sti");

    int result;
    switch (regs->eax) {
        case SYS_WRITE:
            result = sys_write(regs->ebx);
            break;
        case SYS_EXIT:
            task_exit(); /* never returns */
        default:
            result = SYS_ENOSYS;
            break;
    }
    regs->eax = (uint32_t)result; /* restored into eax by the stub's popa */
}

void sys_init(void) {
    isr_register(128, sys_dispatch);
}
