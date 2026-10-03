#include <pit.h>
#include <isr.h>
#include <io.h>

#define PIT_CH0 0x40
#define PIT_COMMAND  0x43
#define PIT_BASE_HZ  1193182

static volatile uint32_t ticks;
static isr_handler_t tick_callback;

static void pit_irq(registers_t *regs) {
    ticks++;
    if (tick_callback != 0) {
        tick_callback(regs);
    }
}

void pit_set_callback(isr_handler_t callback) {
    tick_callback = callback;
}

uint32_t pit_ticks(void) {
    return ticks;
}

void pit_init(uint32_t hz) {
    uint32_t divisor = PIT_BASE_HZ / hz;

    outb(PIT_COMMAND, 0x36); /* channel 0, lobyte/hibyte, rate generator */
    outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));

    irq_register(0, pit_irq);
}
