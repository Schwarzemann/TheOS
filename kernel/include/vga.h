#ifndef THEOS_VGA_H
#define THEOS_VGA_H

#include <stddef.h>

#define VGA_COLS 80
#define VGA_ROWS 25
#define VGA_ATTR 0x1F
#define VGA_MEMORY ((volatile unsigned short *)0xB8000)

void vga_clear(void);
void vga_putc(char c);
void vga_print(const char *str);
void vga_print_n(const char *data, size_t n);
void vga_print_uint(unsigned int value);
void vga_print_hex(unsigned int value);
void vga_set_cursor(int row, int col);

#endif
