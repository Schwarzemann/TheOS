// TheOS - kernel entry point

#include <vga.h>
#include <serial.h>
#include <gdt.h>
#include <isr.h>
#include <pic.h>
#include <pit.h>
#include <keyboard.h>
#include <pmm.h>
#include <paging.h>
#include <heap.h>
#include <panic.h>
#include <task.h>
#include <syscall.h>
#include <usermode.h>
#include <ata.h>
#include <fs.h>
#include <string.h>
#include <boot_info.h>
#include <bin.h>
#include <cwd.h>

#define CMD_BUF_SIZE 64
#define PATH_SIZE 128

/* TOFS is always mounted as drive "T"; TheOS doesn't support multiple drives yet. */
static void print_prompt(void) {
    char path[PATH_SIZE];
    cwd_path(path, sizeof(path));
    vga_print("T:");
    vga_print(path);
    vga_print("> ");
}

static void print_verse(void) {
    vga_print("John 1:1-5 \n");
    vga_print("In the beginning was the Word, and the Word was with God, and the\n");
    vga_print("Word was God. The same was in the beginning with God. All things\n");
    vga_print("were made by him; and without him was not any thing made that was\n");
    vga_print("made. In him was life; and the life was the light of men. And the\n");
    vga_print("light shineth in darkness; and the darkness comprehended it not.\n\n");
}

static void shell(void) {
    print_verse();
    vga_print("TheOS booted. Type 'help' for a list of commands.\n");
    print_prompt();

    char buf[CMD_BUF_SIZE];
    int len = 0;

    for (;;) {
        char c = kbd_getc();
        if (c == '\n') {
            buf[len] = '\0';
            vga_putc('\n');
            run_command(buf);
            len = 0;
            print_prompt();
        } else if (c == '\b') {
            if (len > 0) {
                len--;
                vga_putc('\b');
            }
        } else if (len < CMD_BUF_SIZE - 1) {
            buf[len++] = c;
            vga_putc(c);
        }
    }
}

static void heap_self_test(void) {
    void *a = kmalloc(128);
    void *b = kmalloc(256);
    void *c = kmalloc(64);
    if (a == 0 || b == 0 || c == 0) {
        panic("heap_self_test: kmalloc returned NULL");
    }
    memset(a, 0xAA, 128);
    memset(b, 0xBB, 256);
    if (((unsigned char *)a)[0] != 0xAA || ((unsigned char *)b)[0] != 0xBB) {
        panic("heap_self_test: memory corruption");
    }
    kfree(b);
    kfree(a);
    kfree(c);
    serial_write("TheOS: heap self-test passed\n");
}

void kernel_main(void) {
    serial_init();
    serial_write("TheOS: kernel_main entered\n");

    vga_clear();

    gdt_init();
    serial_write("TheOS: GDT+TSS installed\n");

    isr_init();
    serial_write("TheOS: IDT installed\n");

    pic_remap();
    serial_write("TheOS: PIC remapped to 0x20/0x28\n");

    pit_init(PIT_HZ);
    serial_write("TheOS: PIT timer running at ");
    serial_uint(PIT_HZ);
    serial_write("Hz\n");

    kbd_init();
    serial_write("TheOS: keyboard driver ready\n");

    pmm_init(BOOT_MEM_KB);
    serial_write("TheOS: PMM ready, ");
    serial_uint(pmm_free_count());
    serial_write(" free frames\n");

    paging_init();

    heap_init();
    heap_self_test();

    sys_init();
    serial_write("TheOS: syscalls (int 0x80) ready\n");

    fs_init();
    cwd_init();

    tasks_init();
    task_create("shell", shell);
    user_demo_spawn();

    __asm__ volatile ("sti");
    serial_write("TheOS: interrupts enabled, starting scheduler\n");

    tasks_start();
}
