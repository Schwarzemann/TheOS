#include <idt.h>
#include <gdt.h>
#include <string.h>
#include <stdint.h>

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

#define IDT_ENTRIES 256

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idtp;

static void idt_set_gate(int n, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[n].base_low  = (uint16_t)(base & 0xFFFF);
    idt[n].base_high = (uint16_t)((base >> 16) & 0xFFFF);
    idt[n].sel = sel;
    idt[n].always0 = 0;
    idt[n].flags = flags;
}

extern void isr0();  extern void isr1();  extern void isr2();  extern void isr3();
extern void isr4();  extern void isr5();  extern void isr6();  extern void isr7();
extern void isr8();  extern void isr9();  extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();
extern void isr128();

extern void irq0();  extern void irq1();  extern void irq2();  extern void irq3();
extern void irq4();  extern void irq5();  extern void irq6();  extern void irq7();
extern void irq8();  extern void irq9();  extern void irq10(); extern void irq11();
extern void irq12(); extern void irq13(); extern void irq14(); extern void irq15();

void idt_init(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)&idt;
    memset(&idt, 0, sizeof(idt));

    uint32_t isrs[32] = {
        (uint32_t)isr0,  (uint32_t)isr1,  (uint32_t)isr2,  (uint32_t)isr3,
        (uint32_t)isr4,  (uint32_t)isr5,  (uint32_t)isr6,  (uint32_t)isr7,
        (uint32_t)isr8,  (uint32_t)isr9,  (uint32_t)isr10, (uint32_t)isr11,
        (uint32_t)isr12, (uint32_t)isr13, (uint32_t)isr14, (uint32_t)isr15,
        (uint32_t)isr16, (uint32_t)isr17, (uint32_t)isr18, (uint32_t)isr19,
        (uint32_t)isr20, (uint32_t)isr21, (uint32_t)isr22, (uint32_t)isr23,
        (uint32_t)isr24, (uint32_t)isr25, (uint32_t)isr26, (uint32_t)isr27,
        (uint32_t)isr28, (uint32_t)isr29, (uint32_t)isr30, (uint32_t)isr31,
    };
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, isrs[i], GDT_KCODE, 0x8E);
    }

    /* 0xEE: present, ring3-callable, 32-bit interrupt gate - lets a
     * ring3 task raise int 0x80 for syscalls. */
    idt_set_gate(128, (uint32_t)isr128, GDT_KCODE, 0xEE);

    uint32_t irqs[16] = {
        (uint32_t)irq0,  (uint32_t)irq1,  (uint32_t)irq2,  (uint32_t)irq3,
        (uint32_t)irq4,  (uint32_t)irq5,  (uint32_t)irq6,  (uint32_t)irq7,
        (uint32_t)irq8,  (uint32_t)irq9,  (uint32_t)irq10, (uint32_t)irq11,
        (uint32_t)irq12, (uint32_t)irq13, (uint32_t)irq14, (uint32_t)irq15,
    };
    for (int i = 0; i < 16; i++) {
        idt_set_gate(32 + i, irqs[i], GDT_KCODE, 0x8E);
    }

    __asm__ volatile ("lidt %0" : : "m"(idtp));
}
