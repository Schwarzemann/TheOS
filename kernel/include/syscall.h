#ifndef THEOS_SYSCALL_H
#define THEOS_SYSCALL_H

#define SYS_WRITE 1   /* ebx = pointer to a null-terminated string */
#define SYS_EXIT  2   /* terminates the calling task */

void sys_init(void);

#endif
