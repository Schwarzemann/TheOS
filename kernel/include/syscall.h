#ifndef THEOS_SYSCALL_H
#define THEOS_SYSCALL_H

/* int 0x80: eax = syscall number, ebx = first argument. The result comes back
 * in eax: >= 0 on success, or one of the negative SYS_E* codes. */

#define SYS_WRITE 1   /* ebx = NUL-terminated string; returns its length */
#define SYS_EXIT  2   /* terminates the calling task */

#define SYS_EFAULT -1 /* a pointer argument isn't readable user memory */
#define SYS_ENOSYS -2 /* no such syscall */
#define SYS_EINVAL -3 /* an argument is out of range */

#define SYS_WRITE_MAX 1024 /* longest string SYS_WRITE accepts */

void sys_init(void);

#endif
