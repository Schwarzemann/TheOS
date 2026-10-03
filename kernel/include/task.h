#ifndef THEOS_TASK_H
#define THEOS_TASK_H

#include <stdint.h>

#define TASK_NAME_LEN 16
#define TASK_MAX 8

typedef enum { TASK_READY, TASK_DEAD } task_state_t;

typedef struct task {
    uint32_t esp;          /* saved stack pointer, valid when not current */
    uint32_t kstack_top;   /* top of this task's kernel stack - for TSS.esp0 */
    task_state_t state;
    char name[TASK_NAME_LEN];
} task_t;

/* Call after heap_init() (stacks come from kmalloc) and before tasks_start(). */
void tasks_init(void);

/* entry is run at ring0. Returns the new task's id, or -1 if the task
 * table is full. */
int task_create(const char *name, void (*entry)(void));

/* Marks the currently running task dead and switches away. Never
 * returns. */
__attribute__((noreturn)) void task_exit(void);

/* Starts round-robin scheduling (driven by the PIT tick) and enters the first task. */
__attribute__((noreturn)) void tasks_start(void);

#endif
