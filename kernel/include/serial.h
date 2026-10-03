#ifndef THEOS_SERIAL_H
#define THEOS_SERIAL_H

/* COM1 debug log. Run with `make run-headless`. */

void serial_init(void);
void serial_putc(char c);
void serial_write(const char *str);
void serial_uint(unsigned int value);
void serial_hex(unsigned int value);

#endif
