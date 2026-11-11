#include <sync.h>
#include <task.h>
#include <panic.h>

void mutex_lock(mutex_t *m) {
    uint32_t flags = irq_save();
    if (m->locked && m->owner == task_current()) {
        panic("mutex_lock: task already holds this lock");
    }
    while (m->locked) {
        task_block_on(m);
    }
    m->locked = 1;
    m->owner = task_current();
    task_locks_changed(1);
    irq_restore(flags);
}

void mutex_unlock(mutex_t *m) {
    uint32_t flags = irq_save();
    if (!m->locked || m->owner != task_current()) {
        panic("mutex_unlock: lock not held by this task");
    }
    task_locks_changed(-1);
    m->locked = 0;
    task_wake_all(m);
    irq_restore(flags);
}
