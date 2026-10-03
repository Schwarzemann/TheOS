#ifndef THEOS_BOOT_INFO_H
#define THEOS_BOOT_INFO_H

#define BOOT_MEM_KB  (*(volatile unsigned int *)0x0500)
#define BOOT_DISK_KB (*(volatile unsigned int *)0x0504)

#endif
