#include <paging.h>
#include <pmm.h>
#include <isr.h>
#include <panic.h>
#include <serial.h>
#include <string.h>
#include <sync.h>
#include <task.h>

#define PT_ENTRIES 1024
#define FRAME_MASK 0xFFFFF000u
#define CR0_WP 0x00010000u
#define CR0_PG 0x80000000u

/* Ring3 region of the kernel image, from linker.ld. */
extern char user_text_start[], user_text_end[];
extern char user_data_start[], user_data_end[];

/* Only the directory is static; page tables come from the PMM on demand, so the
 * map doesn't cost memory proportional to PMM_MAX_FRAMES. Every directory entry
 * allows user access, so each page's own entry alone decides it. */
static uint32_t page_dir[PT_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

/* Page tables live in identity-mapped RAM, so their physical address works as a
 * pointer both before and after paging is enabled. */
static uint32_t *table_for(uint32_t virt, int create) {
    uint32_t *pde = &page_dir[virt >> 22];
    if (*pde & PAGE_PRESENT) {
        return (uint32_t *)(*pde & FRAME_MASK);
    }
    if (!create) {
        return 0;
    }
    uint32_t pt = pmm_alloc();
    if (pt == 0) {
        return 0;
    }
    memset((void *)pt, 0, PAGE_SIZE);
    *pde = pt | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    return (uint32_t *)pt;
}

static void invlpg(uint32_t virt) {
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

int paging_map(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t irq = irq_save();
    uint32_t *pt = table_for(virt, 1);
    if (pt == 0) {
        irq_restore(irq);
        return -1;
    }
    pt[(virt >> 12) & (PT_ENTRIES - 1)] = (phys & FRAME_MASK) | (flags & ~FRAME_MASK) | PAGE_PRESENT;
    invlpg(virt);
    irq_restore(irq);
    return 0;
}

void paging_unmap(uint32_t virt) {
    uint32_t irq = irq_save();
    uint32_t *pt = table_for(virt, 0);
    if (pt != 0) {
        pt[(virt >> 12) & (PT_ENTRIES - 1)] = 0;
        invlpg(virt);
    }
    irq_restore(irq);
}

uint32_t paging_lookup(uint32_t virt) {
    uint32_t irq = irq_save();
    uint32_t *pt = table_for(virt, 0);
    uint32_t pte = pt != 0 ? pt[(virt >> 12) & (PT_ENTRIES - 1)] : 0;
    irq_restore(irq);
    return (pte & PAGE_PRESENT) ? pte : 0;
}

int paging_user_ok(uint32_t addr, uint32_t len, int write) {
    if (len == 0) {
        return 1;
    }
    uint32_t last = addr + len - 1;
    if (last < addr) {
        return 0; /* wraps around the top of the address space */
    }
    uint32_t need = PAGE_PRESENT | PAGE_USER | (write ? PAGE_WRITE : 0);
    for (uint32_t page = addr & FRAME_MASK; ; page += PAGE_SIZE) {
        if ((paging_lookup(page) & need) != need) {
            return 0;
        }
        if (page == (last & FRAME_MASK)) {
            return 1;
        }
    }
}

static void page_fault(registers_t *regs) {
    uint32_t fault_addr;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(fault_addr));

    char buf[9];
    char msg[160];
    msg[0] = '\0';
    strcat(msg, "page fault at 0x");
    utoa(fault_addr, buf, 16);
    strcat(msg, buf);
    strcat(msg, (regs->err_code & 0x1) ? " (protection violation" : " (page not present");
    strcat(msg, (regs->err_code & 0x2) ? ", write" : ", read");
    strcat(msg, (regs->err_code & 0x4) ? ", user mode)" : ", kernel mode)");
    strcat(msg, " eip=0x");
    utoa(regs->eip, buf, 16);
    strcat(msg, buf);

    /* A ring3 fault is the program's bug; only a ring0 one is the kernel's. */
    if (regs->err_code & 0x4) {
        task_kill_current(msg);
    }

    serial_write("PAGE FAULT cr2=");
    serial_hex(fault_addr);
    serial_write(" err=");
    serial_hex(regs->err_code);
    serial_write("\n");
    panic(msg);
}

static void map_range(char *start, char *end, uint32_t flags) {
    for (uint32_t va = (uint32_t)start; va < (uint32_t)end; va += PAGE_SIZE) {
        if (paging_map(va, va, flags) < 0) {
            panic("paging_init: out of memory for page tables");
        }
    }
}

void paging_init(void) {
    uint32_t total_frames = pmm_total();
    if ((uint64_t)total_frames * PAGE_SIZE > USER_STACKS_BASE) {
        panic("paging_init: RAM overlaps USER_STACKS_BASE");
    }

    for (uint32_t d = 0; d < PT_ENTRIES; d++) {
        page_dir[d] = 0;
    }

    /* Frame 0 stays unmapped so NULL dereferences fault. */
    for (uint32_t frame = 1; frame < total_frames; frame++) {
        if (paging_map(frame * PAGE_SIZE, frame * PAGE_SIZE, PAGE_WRITE) < 0) {
            panic("paging_init: out of memory for page tables");
        }
    }
    map_range(user_text_start, user_text_end, PAGE_USER);
    map_range(user_data_start, user_data_end, PAGE_USER | PAGE_WRITE);

    isr_register(14, page_fault);

    /* WP makes read-only pages read-only for ring0 as well. */
    __asm__ volatile (
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or %1, %%eax\n"
        "mov %%eax, %%cr0\n"
        : : "r"(page_dir), "i"(CR0_PG | CR0_WP) : "eax", "memory"
    );

    serial_write("TheOS: paging enabled, identity-mapped ");
    serial_uint(total_frames * PAGE_SIZE / (1024 * 1024));
    serial_write(" MiB, kernel pages supervisor-only\n");
}
