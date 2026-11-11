#include <task.h>
#include <heap.h>
#include <gdt.h>
#include <pit.h>
#include <isr.h>
#include <panic.h>
#include <serial.h>
#include <string.h>
#include <sync.h>
#include <paging.h>
#include <pmm.h>
#include <vga.h>

#define TASK_STACK_SZ (16 * 1024)
#define IDLE_TASK 0

/* 64 KiB of address space per slot; at most 15 pages are mapped at the bottom of
 * it, so there's always an unmapped page above each stack and below the next. */
#define USER_STACK_SLOT ((TASK_USER_STACK_MAX_PAGES + 1) * PAGE_SIZE)

extern void task_switch(uint32_t *old_esp_store, uint32_t new_esp);

static task_t tasks[TASK_MAX];
static int current_task = -1;

static uint32_t user_stack_base(int id) {
    return USER_STACKS_BASE + (uint32_t)id * USER_STACK_SLOT;
}

static void unmap_user_stack(int id) {
    for (uint32_t i = 0; i < tasks[id].user_stack_pages; i++) {
        uint32_t va = user_stack_base(id) + i * PAGE_SIZE;
        uint32_t pte = paging_lookup(va);
        paging_unmap(va);
        if (pte != 0) {
            pmm_free(pte & ~(uint32_t)(PAGE_SIZE - 1));
        }
    }
    tasks[id].user_stack_pages = 0;
}

/* Frees the stacks and slots of dead tasks. A task can't free the stack it is
 * running on, so this happens later, from another task. */
static void tasks_reap(void) {
    uint32_t flags = irq_save();
    for (int i = 0; i < TASK_MAX; i++) {
        if (tasks[i].state == TASK_DEAD && i != current_task) {
            kfree(tasks[i].kstack);
            unmap_user_stack(i);
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

uint32_t task_map_user_stack(uint32_t pages) {
    task_t *t = &tasks[current_task];
    if (pages == 0 || pages > TASK_USER_STACK_MAX_PAGES || t->user_stack_pages != 0) {
        return 0;
    }
    uint32_t base = user_stack_base(current_task);
    for (uint32_t i = 0; i < pages; i++) {
        uint32_t frame = pmm_alloc();
        if (frame != 0) {
            /* Zeroed through the identity map, so no old kernel data reaches ring3. */
            memset((void *)frame, 0, PAGE_SIZE);
            if (paging_map(base + i * PAGE_SIZE, frame, PAGE_USER | PAGE_WRITE) < 0) {
                pmm_free(frame);
                frame = 0;
            }
        }
        if (frame == 0) {
            unmap_user_stack(current_task);
            return 0;
        }
        t->user_stack_pages = i + 1;
    }
    return base + pages * PAGE_SIZE;
}

void task_locks_changed(int delta) {
    if (current_task >= 0) {
        tasks[current_task].locks_held += delta;
    }
}

__attribute__((noreturn)) void task_kill_current(const char *reason) {
    char msg[224];
    msg[0] = '\0';
    strcat(msg, "[kernel] killed task '");
    strcat(msg, tasks[current_task].name);
    strcat(msg, "': ");
    strncpy(msg + strlen(msg), reason, sizeof(msg) - strlen(msg) - 2);
    msg[sizeof(msg) - 2] = '\0';
    strcat(msg, "\n");
    vga_print(msg);
    task_exit();
}

__attribute__((noreturn)) void task_exit(void) {
    __asm__ volatile ("cli");
    if (tasks[current_task].locks_held != 0) {
        panic("task_exit: task still holds a lock");
    }
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
