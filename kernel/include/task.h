#ifndef THEOS_TASK_H
#define THEOS_TASK_H

#include <stddef.h>
#include <stdint.h>

#define TASK_NAME_LEN 16
#define TASK_MAX 8     /* includes the idle task */

typedef enum {
    TASK_FREE = 0,     /* unused slot */
    TASK_READY,
    TASK_BLOCKED,      /* waiting for task_wake_all() on wait_chan */
    TASK_DEAD          /* exited; stacks are freed by the reaper */
} task_state_t;

typedef struct task {
    uint32_t esp;          /* saved stack pointer, valid when not current */
    uint32_t kstack_top;   /* top of this task's kernel stack - for TSS.esp0 */
    void *kstack;          /* kmalloc'd kernel stack, freed once the task is dead */
    void *user_stack;      /* kmalloc'd ring3 stack, if the task has one */
    const void *wait_chan; /* what a TASK_BLOCKED task is waiting on */
    task_state_t state;
    char name[TASK_NAME_LEN];
} task_t;

/* Call after heap_init() (stacks come from kmalloc) and before tasks_start().
 * Creates the idle task, which runs only when nothing else can. */
void tasks_init(void);

/* entry is run at ring0, and returning from it exits the task. Returns the new
 * task's id, or -1 if the task table is full. */
int task_create(const char *name, void (*entry)(void));

/* The running task's id, or -1 before tasks_start(). */
int task_current(void);

/* Gives up the rest of this time slice. */
void task_yield(void);

/* Blocks the running task until task_wake_all(chan). Interrupts must be off
 * from checking the awaited condition until this call, or a wakeup can be
 * missed. Wakeups can be spurious, so re-check the condition in a loop. */
void task_block_on(const void *chan);

/* Makes every task blocked on chan runnable. Safe to call from an IRQ handler. */
void task_wake_all(const void *chan);

/* Allocates a ring3 stack that is freed when the running task dies. Returns
 * its lowest address, or 0 if out of memory. */
void *task_alloc_user_stack(size_t size);

/* Marks the currently running task dead and switches away. Never
 * returns. */
__attribute__((noreturn)) void task_exit(void);

/* Starts round-robin scheduling (driven by the PIT tick) and enters the first task. */
__attribute__((noreturn)) void tasks_start(void);

#endif
