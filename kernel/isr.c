#include <isr.h>
#include <idt.h>
#include <pic.h>
#include <panic.h>
#include <serial.h>
#include <string.h>

static isr_handler_t isr_handlers[256];
static isr_handler_t irq_handlers[16];

static const char *exc_msgs[32] = {
    "Division by zero", "Debug", "Non-maskable interrupt", "Breakpoint",
    "Overflow", "Bound range exceeded", "Invalid opcode", "Device not available",
    "Double fault", "Coprocessor segment overrun", "Invalid TSS", "Segment not present",
    "Stack-segment fault", "General protection fault", "Page fault", "Reserved",
    "x87 floating-point exception", "Alignment check", "Machine check", "SIMD floating-point exception",
    "Virtualization exception", "Control protection exception", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor injection exception", "VMM communication exception", "Security exception", "Reserved",
};

void isr_register(int n, isr_handler_t handler) {
    isr_handlers[n] = handler;
}

void irq_register(int irq, isr_handler_t handler) {
    irq_handlers[irq] = handler;
    pic_unmask(irq);
}

void isr_init(void) {
    idt_init();
}

static void dump_and_panic(registers_t *regs) {
    char buf[9];
    serial_write("\nEXCEPTION ");
    serial_uint(regs->int_no);
    serial_write(": ");
    serial_write(exc_msgs[regs->int_no]);
    serial_write(" err=");
    serial_hex(regs->err_code);
    serial_write(" eip=");
    serial_hex(regs->eip);
    serial_write("\n");

    static char msg[192];
    msg[0] = '\0';
    strcat(msg, exc_msgs[regs->int_no]);
    strcat(msg, "\n\nvector=");
    utoa(regs->int_no, buf, 10);
    strcat(msg, buf);
    strcat(msg, "  error_code=0x");
    utoa(regs->err_code, buf, 16);
    strcat(msg, buf);
    strcat(msg, "\neip=0x");
    utoa(regs->eip, buf, 16);
    strcat(msg, buf);
    strcat(msg, "  cs=0x");
    utoa(regs->cs, buf, 16);
    strcat(msg, buf);
    strcat(msg, "\neax=0x");
    utoa(regs->eax, buf, 16);
    strcat(msg, buf);
    strcat(msg, "  ebx=0x");
    utoa(regs->ebx, buf, 16);
    strcat(msg, buf);
    strcat(msg, "  ecx=0x");
    utoa(regs->ecx, buf, 16);
    strcat(msg, buf);
    strcat(msg, "  edx=0x");
    utoa(regs->edx, buf, 16);
    strcat(msg, buf);

    panic(msg);
}

void isr_handler(registers_t *regs) {
    if (regs->int_no < 32) {
        if (isr_handlers[regs->int_no] != 0) {
            isr_handlers[regs->int_no](regs);
            return;
        }
        dump_and_panic(regs);
        return;
    }

    if (isr_handlers[regs->int_no] != 0) {
        isr_handlers[regs->int_no](regs);
    }
}

void irq_handler(registers_t *regs) {
    int irq = (int)regs->int_no - 32;

    if (irq_handlers[irq] != 0) {
        irq_handlers[irq](regs);
    }

    pic_eoi(irq);
}
