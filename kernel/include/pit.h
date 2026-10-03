#ifndef THEOS_PIT_H
#define THEOS_PIT_H

#include <stdint.h>
#include <isr.h>

#define PIT_HZ 100

void pit_init(uint32_t hz);
uint32_t pit_ticks(void);

/* Runs on every tick, after the counter is updated. The scheduler uses it for preemption. */
void pit_set_callback(isr_handler_t callback);

#endif
