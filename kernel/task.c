#include <task.h>
#include <heap.h>
#include <gdt.h>
#include <pit.h>
#include <isr.h>
#include <panic.h>
#include <serial.h>
#include <string.h>

#define TASK_STACK_SZ (16 * 1024)

extern void task_switch(uint32_t *old_esp_store, uint32_t new_esp);

static task_t tasks[TASK_MAX];
static int task_count;
static int current_task = -1;

void tasks_init(void) {
    memset(tasks, 0, sizeof(tasks));
    task_count = 0;
    current_task = -1;
}

int task_create(const char *name, void (*entry)(void)) {
    if (task_count >= TASK_MAX) {
        return -1;
    }

    uint8_t *stack = kmalloc(TASK_STACK_SZ);
    if (stack == 0) {
        panic("task_create: out of heap memory for a task stack");
    }
    uint32_t stack_top = (uint32_t)(stack + TASK_STACK_SZ);

    uint32_t *sp = (uint32_t *)stack_top;
    /* task_switch pops eflags, edi, esi, ebx, ebp, then rets into entry. */
    *(--sp) = (uint32_t)entry;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0x202; /* IF set */

    int id = task_count++;
    tasks[id].esp = (uint32_t)sp;
    tasks[id].kstack_top = stack_top;
    tasks[id].state = TASK_READY;
    strncpy(tasks[id].name, name, TASK_NAME_LEN - 1);

    return id;
}

static int next_ready(int after) {
    for (int i = 1; i <= task_count; i++) {
        int idx = (after + i) % task_count;
        if (tasks[idx].state == TASK_READY) {
            return idx;
        }
    }
    return -1;
}

static void schedule(registers_t *regs) {
    (void)regs;
    if (task_count == 0 || current_task < 0) {
        return;
    }

    int next = next_ready(current_task);
    if (next < 0 || next == current_task) {
        return; /* nothing else runnable */
    }

    int prev = current_task;
    current_task = next;
    gdt_set_kstack(tasks[next].kstack_top);
    task_switch(&tasks[prev].esp, tasks[next].esp);
}

__attribute__((noreturn)) void task_exit(void) {
    __asm__ volatile ("cli");
    tasks[current_task].state = TASK_DEAD;

    int next = next_ready(current_task);
    if (next < 0) {
        panic("task_exit: no runnable tasks left");
    }

    int prev = current_task;
    current_task = next;
    gdt_set_kstack(tasks[next].kstack_top);
    __asm__ volatile ("sti");
    uint32_t discard;
    task_switch(&discard, tasks[next].esp);
    (void)prev;

    for (;;) { } /* unreachable */
}

__attribute__((noreturn)) void tasks_start(void) {
    if (task_count == 0) {
        panic("tasks_start: no tasks created");
    }

    pit_set_callback(schedule);

    current_task = 0;
    gdt_set_kstack(tasks[0].kstack_top);

    serial_write("TheOS: starting scheduler with ");
    serial_uint((unsigned int)task_count);
    serial_write(" task(s)\n");

    uint32_t discard_esp;
    task_switch(&discard_esp, tasks[0].esp);

    for (;;) { } /* unreachable: tasks never return to here */
}
