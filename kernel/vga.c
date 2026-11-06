#include <vga.h>
#include <io.h>
#include <serial.h>
#include <string.h>
#include <sync.h>

#define VGA_CRTC_IDX  0x3D4
#define VGA_CRTC_DATA 0x3D5
#define VGA_CUR_HI    0x0E
#define VGA_CUR_LO    0x0F

static int cursor_row;
static int cursor_col;

/* Guards the cursor and screen, and keeps each print call's text together. */
static mutex_t vga_lock;

static void set_cursor(int row, int col) {
    cursor_row = row;
    cursor_col = col;
    unsigned short pos = (unsigned short)(row * VGA_COLS + col);
    outb(VGA_CRTC_IDX, VGA_CUR_HI);
    outb(VGA_CRTC_DATA, (unsigned char)(pos >> 8));
    outb(VGA_CRTC_IDX, VGA_CUR_LO);
    outb(VGA_CRTC_DATA, (unsigned char)(pos & 0xFF));
}

void vga_set_cursor(int row, int col) {
    mutex_lock(&vga_lock);
    set_cursor(row, col);
    mutex_unlock(&vga_lock);
}

static void put_cell(int row, int col, char c) {
    VGA_MEMORY[row * VGA_COLS + col] = (unsigned short)((VGA_ATTR << 8) | (unsigned char)c);
}

void vga_clear(void) {
    mutex_lock(&vga_lock);
    for (int row = 0; row < VGA_ROWS; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            put_cell(row, col, ' ');
        }
    }
    set_cursor(0, 0);
    mutex_unlock(&vga_lock);
}

static void scroll(void) {
    memmove((void *)VGA_MEMORY, (void *)(VGA_MEMORY + VGA_COLS),
            (size_t)(VGA_ROWS - 1) * VGA_COLS * sizeof(unsigned short));
    for (int col = 0; col < VGA_COLS; col++) {
        put_cell(VGA_ROWS - 1, col, ' ');
    }
    cursor_row = VGA_ROWS - 1;
}

static void putc_unlocked(char c) {
    serial_putc(c); /* mirror the screen console to the serial debug log */

    if (c == '\n') {
        cursor_row++;
        cursor_col = 0;
    } else if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
        } else if (cursor_row > 0) {
            cursor_row--;
            cursor_col = VGA_COLS - 1;
        }
        put_cell(cursor_row, cursor_col, ' ');
    } else {
        put_cell(cursor_row, cursor_col, c);
        cursor_col++;
        if (cursor_col >= VGA_COLS) {
            cursor_col = 0;
            cursor_row++;
        }
    }
    if (cursor_row >= VGA_ROWS) {
        scroll();
    }
    set_cursor(cursor_row, cursor_col);
}

void vga_putc(char c) {
    mutex_lock(&vga_lock);
    putc_unlocked(c);
    mutex_unlock(&vga_lock);
}

void vga_print(const char *str) {
    mutex_lock(&vga_lock);
    for (size_t i = 0; str[i] != '\0'; i++) {
        putc_unlocked(str[i]);
    }
    mutex_unlock(&vga_lock);
}

void vga_print_n(const char *data, size_t n) {
    mutex_lock(&vga_lock);
    for (size_t i = 0; i < n; i++) {
        putc_unlocked(data[i]);
    }
    mutex_unlock(&vga_lock);
}

void vga_print_uint(unsigned int value) {
    char buf[11];
    utoa(value, buf, 10);
    vga_print(buf);
}

void vga_print_hex(unsigned int value) {
    char buf[9];
    utoa(value, buf, 16);
    vga_print("0x");
    vga_print(buf);
}
