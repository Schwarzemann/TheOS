#include <task.h>
#include <heap.h>
#include <gdt.h>
#include <pit.h>
#include <isr.h>
#include <panic.h>
#include <serial.h>
#include <string.h>
#include <sync.h>

#define TASK_STACK_SZ (16 * 1024)
#define IDLE_TASK 0

extern void task_switch(uint32_t *old_esp_store, uint32_t new_esp);

static task_t tasks[TASK_MAX];
static int current_task = -1;

/* Frees the stacks and slots of dead tasks. A task can't free the stack it is
 * running on, so this happens later, from another task. */
static void tasks_reap(void) {
    uint32_t flags = irq_save();
    for (int i = 0; i < TASK_MAX; i++) {
        if (tasks[i].state == TASK_DEAD && i != current_task) {
            kfree(tasks[i].kstack);
            kfree(tasks[i].user_stack);
            memset(&tasks[i], 0, sizeof(tasks[i]));
        }
    }
    irq_restore(flags);
}

static void idle_entry(void) {
    for (;;) {
        tasks_reap();
        __asm__ volatile ("sti; hlt");
        task_yield(); /* run whatever that interrupt woke, without waiting for a tick */
    }
}

void tasks_init(void) {
    memset(tasks, 0, sizeof(tasks));
    current_task = -1;
    task_create("idle", idle_entry); /* first free slot, so id == IDLE_TASK */
}

int task_create(const char *name, void (*entry)(void)) {
    uint32_t flags = irq_save();
    tasks_reap();

    int id = -1;
    for (int i = 0; i < TASK_MAX; i++) {
        if (tasks[i].state == TASK_FREE) {
            id = i;
            break;
        }
    }
    if (id < 0) {
        irq_restore(flags);
        return -1;
    }

    uint8_t *stack = kmalloc(TASK_STACK_SZ);
    if (stack == 0) {
        panic("task_create: out of heap memory for a task stack");
    }
    uint32_t stack_top = (uint32_t)(stack + TASK_STACK_SZ);

    uint32_t *sp = (uint32_t *)stack_top;
    /* task_switch pops eflags, edi, esi, ebx, ebp, then rets into entry.
     * If entry returns, its own ret lands in task_exit. */
    *(--sp) = (uint32_t)task_exit;
    *(--sp) = (uint32_t)entry;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0x202; /* IF set */

    tasks[id].esp = (uint32_t)sp;
    tasks[id].kstack_top = stack_top;
    tasks[id].kstack = stack;
    tasks[id].state = TASK_READY;
    strncpy(tasks[id].name, name, TASK_NAME_LEN - 1);

    irq_restore(flags);
    return id;
}

int task_current(void) {
    return current_task;
}

/* Round-robin over everything but idle, which runs only as a last resort. */
static int next_ready(void) {
    for (int i = 1; i <= TASK_MAX; i++) {
        int idx = (current_task + i) % TASK_MAX;
        if (idx != IDLE_TASK && tasks[idx].state == TASK_READY) {
            return idx;
        }
    }
    return IDLE_TASK;
}

/* Interrupts must be off. Each task's interrupt state is saved and restored by
 * task_switch, so this returns with them still off. */
static void switch_to_next(void) {
    int next = next_ready();
    if (next == current_task) {
        return;
    }

    int prev = current_task;
    current_task = next;
    gdt_set_kstack(tasks[next].kstack_top);
    task_switch(&tasks[prev].esp, tasks[next].esp);
}

static void schedule(registers_t *regs) {
    (void)regs;
    switch_to_next();
}

void task_yield(void) {
    uint32_t flags = irq_save();
    switch_to_next();
    irq_restore(flags);
}

void task_block_on(const void *chan) {
    if (current_task < 0) {
        panic("task_block_on: called before the scheduler started");
    }
    tasks[current_task].wait_chan = chan;
    tasks[current_task].state = TASK_BLOCKED;
    switch_to_next();
}

void task_wake_all(const void *chan) {
    for (int i = 0; i < TASK_MAX; i++) {
        if (tasks[i].state == TASK_BLOCKED && tasks[i].wait_chan == chan) {
            tasks[i].wait_chan = 0;
            tasks[i].state = TASK_READY;
        }
    }
}

void *task_alloc_user_stack(size_t size) {
    void *stack = kmalloc(size);
    tasks[current_task].user_stack = stack;
    return stack;
}

__attribute__((noreturn)) void task_exit(void) {
    __asm__ volatile ("cli");
    tasks[current_task].state = TASK_DEAD;
    switch_to_next();

    for (;;) { } /* unreachable: a dead task is never scheduled again */
}

__attribute__((noreturn)) void tasks_start(void) {
    /* Off until the first task's saved EFLAGS turns them on: a tick in between
     * would save this boot stack as a task's context. */
    __asm__ volatile ("cli");

    current_task = IDLE_TASK;
    int first = next_ready();
    if (first == IDLE_TASK) {
        panic("tasks_start: no tasks created");
    }

    pit_set_callback(schedule);

    current_task = first;
    gdt_set_kstack(tasks[first].kstack_top);

    serial_write("TheOS: starting scheduler\n");

    uint32_t discard_esp;
    task_switch(&discard_esp, tasks[first].esp);

    for (;;) { } /* unreachable: tasks never return to here */
}
