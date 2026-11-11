#ifndef THEOS_TASK_H
#define THEOS_TASK_H

#include <stddef.h>
#include <stdint.h>

#define TASK_NAME_LEN 16
#define TASK_MAX 8     /* includes the idle task */
#define TASK_USER_STACK_MAX_PAGES 15

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
    uint32_t user_stack_pages; /* ring3 stack pages mapped by task_map_user_stack */
    int locks_held;        /* mutexes held; exiting with any held is a bug */
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

/* Maps a ring3 stack of `pages` pages (at most TASK_USER_STACK_MAX_PAGES) for the
 * running task, freed when it dies. Each task slot has its own range above
 * USER_STACKS_BASE with unmapped pages on both sides, so overflowing the stack
 * faults. Returns the stack's top address, or 0 if out of memory. */
uint32_t task_map_user_stack(uint32_t pages);

/* Kills the running task over a fault in its ring3 code: reports `reason` on
 * the console and exits. Never returns. */
__attribute__((noreturn)) void task_kill_current(const char *reason);

/* For mutex_lock/unlock: adjusts the running task's held-lock count. */
void task_locks_changed(int delta);

/* Marks the currently running task dead and switches away. Never
 * returns. */
__attribute__((noreturn)) void task_exit(void);

/* Starts round-robin scheduling (driven by the PIT tick) and enters the first task. */
__attribute__((noreturn)) void tasks_start(void);

#endif
