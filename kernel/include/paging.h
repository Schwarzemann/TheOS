#ifndef THEOS_PAGING_H
#define THEOS_PAGING_H

/* Identity-maps all RAM and enables paging. Needs pmm_init() and isr_init() first.
 * No demand paging: every mapped page is backed by a frame up front. */
void paging_init(void);

#endif
