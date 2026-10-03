#ifndef THEOS_HEAP_H
#define THEOS_HEAP_H

#include <stddef.h>

/* Must run after paging_init(): carves a heap arena out of identity-
 * mapped RAM by pulling frames straight from the PMM. */
void heap_init(void);

void *kmalloc(size_t size);
void kfree(void *ptr);

size_t heap_free_bytes(void);

#endif
