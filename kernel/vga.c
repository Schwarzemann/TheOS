#include <vga.h>
#include <io.h>
#include <serial.h>
#include <string.h>

#define VGA_CRTC_IDX  0x3D4
#define VGA_CRTC_DATA 0x3D5
#define VGA_CUR_HI    0x0E
#define VGA_CUR_LO    0x0F

static int cursor_row;
static int cursor_col;

void vga_set_cursor(int row, int col) {
    cursor_row = row;
    cursor_col = col;
    unsigned short pos = (unsigned short)(row * VGA_COLS + col);
    outb(VGA_CRTC_IDX, VGA_CUR_HI);
    outb(VGA_CRTC_DATA, (unsigned char)(pos >> 8));
    outb(VGA_CRTC_IDX, VGA_CUR_LO);
    outb(VGA_CRTC_DATA, (unsigned char)(pos & 0xFF));
}

static void put_cell(int row, int col, char c) {
    VGA_MEMORY[row * VGA_COLS + col] = (unsigned short)((VGA_ATTR << 8) | (unsigned char)c);
}

void vga_clear(void) {
    for (int row = 0; row < VGA_ROWS; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            put_cell(row, col, ' ');
        }
    }
    vga_set_cursor(0, 0);
}

static void scroll(void) {
    memmove((void *)VGA_MEMORY, (void *)(VGA_MEMORY + VGA_COLS),
            (size_t)(VGA_ROWS - 1) * VGA_COLS * sizeof(unsigned short));
    for (int col = 0; col < VGA_COLS; col++) {
        put_cell(VGA_ROWS - 1, col, ' ');
    }
    cursor_row = VGA_ROWS - 1;
}

void vga_putc(char c) {
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
    vga_set_cursor(cursor_row, cursor_col);
}

void vga_print(const char *str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        vga_putc(str[i]);
    }
}

void vga_print_n(const char *data, size_t n) {
    for (size_t i = 0; i < n; i++) {
        vga_putc(data[i]);
    }
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
