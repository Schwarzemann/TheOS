#ifndef THEOS_SYNC_H
#define THEOS_SYNC_H

#include <stdint.h>

/* Single CPU, so turning interrupts off is enough for a critical section.
 * Use this for short sections, and for data shared with IRQ handlers. */
static inline uint32_t irq_save(void) {
    uint32_t flags;
    __asm__ volatile ("pushf; pop %0; cli" : "=r"(flags) : : "memory");
    return flags;
}

static inline void irq_restore(uint32_t flags) {
    if (flags & 0x200) {
        __asm__ volatile ("sti" : : : "memory");
    }
}

/* Sleeping lock for long sections (console output, disk I/O). Zero-initialized
 * is unlocked. Task context only: an IRQ handler must never take one. */
typedef struct {
    volatile int locked;
    int owner;
} mutex_t;

void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);

#endif
