#include <paging.h>
#include <pmm.h>
#include <isr.h>
#include <panic.h>
#include <serial.h>
#include <string.h>

#define PAGE_PRESENT 0x1
#define PAGE_WRITE   0x2
/* No per-task address spaces yet: ring3 tasks share the kernel's identity
 * map, so every page needs PAGE_USER or a ring3 fetch faults immediately. */
#define PAGE_USER    0x4
#define PT_ENTRIES 1024
#define PT_BYTES (PT_ENTRIES * PMM_FRAME_SIZE)

/* Only the directory is static; page tables come from the PMM on demand,
 * so the map doesn't cost memory proportional to PMM_MAX_FRAMES. */
static uint32_t page_dir[PT_ENTRIES] __attribute__((aligned(4096)));

static void page_fault(registers_t *regs) {
    uint32_t fault_addr;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(fault_addr));

    char buf[9];
    static char msg[160];
    msg[0] = '\0';
    strcat(msg, "Page fault at 0x");
    utoa(fault_addr, buf, 16);
    strcat(msg, buf);
    strcat(msg, (regs->err_code & 0x1) ? "\n(protection violation" : "\n(page not present");
    strcat(msg, (regs->err_code & 0x2) ? ", write" : ", read");
    strcat(msg, (regs->err_code & 0x4) ? ", user mode)" : ", kernel mode)");
    strcat(msg, "\neip=0x");
    utoa(regs->eip, buf, 16);
    strcat(msg, buf);

    serial_write("PAGE FAULT cr2=");
    serial_hex(fault_addr);
    serial_write(" err=");
    serial_hex(regs->err_code);
    serial_write("\n");

    panic(msg);
}

void paging_init(void) {
    uint32_t total_frames = pmm_total();
    uint32_t mapped_bytes = total_frames * PMM_FRAME_SIZE;
    uint32_t dirs_needed = (mapped_bytes + PT_BYTES - 1) / PT_BYTES;
    if (dirs_needed > PT_ENTRIES) {
        dirs_needed = PT_ENTRIES;
    }

    for (uint32_t d = 0; d < PT_ENTRIES; d++) {
        page_dir[d] = 0;
    }

    for (uint32_t d = 0; d < dirs_needed; d++) {
        uint32_t pt_phys = pmm_alloc();
        if (pt_phys == 0) {
            panic("paging_init: out of memory for page tables");
        }
        uint32_t *pt = (uint32_t *)pt_phys; /* paging isn't enabled yet: phys == virt */
        for (uint32_t t = 0; t < PT_ENTRIES; t++) {
            uint32_t phys = d * PT_BYTES + t * PMM_FRAME_SIZE;
            pt[t] = phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        }
        page_dir[d] = pt_phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }

    isr_register(14, page_fault);

    __asm__ volatile (
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n"
        : : "r"(page_dir) : "eax"
    );

    serial_write("TheOS: paging enabled, identity-mapped ");
    serial_uint(mapped_bytes / (1024 * 1024));
    serial_write(" MiB\n");
}
