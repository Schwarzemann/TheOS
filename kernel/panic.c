#include <panic.h>
#include <serial.h>
#include <string.h>
#include <vga.h>

#define PANIC_ATTR 0x4F /* white on red */

/* Panic can fire from any state, so it keeps its own cursor. */
static int row, col;

static void panic_putc(char c) {
    if (c == '\n') {
        row++;
        col = 0;
        return;
    }
    VGA_MEMORY[row * VGA_COLS + col] = (unsigned short)((PANIC_ATTR << 8) | (unsigned char)c);
    col++;
    if (col >= VGA_COLS) {
        col = 0;
        row++;
    }
}

static void panic_print(const char *str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        panic_putc(str[i]);
    }
}

void panic(const char *msg) {
    __asm__ volatile ("cli");

    for (int i = 0; i < VGA_ROWS * VGA_COLS; i++) {
        VGA_MEMORY[i] = (unsigned short)((PANIC_ATTR << 8) | ' ');
    }
    row = 0;
    col = 0;

    panic_print("*** TheOS kernel panic ***\n\n");
    panic_print(msg);
    panic_print("\n\nSystem halted.");

    serial_write("\n*** KERNEL PANIC ***\n");
    serial_write(msg);
    serial_write("\n");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
