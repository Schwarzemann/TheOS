#include <serial.h>
#include <io.h>
#include <string.h>

#define COM1 0x3F8

void serial_init(void) {
    outb(COM1 + 1, 0x00); /* disable interrupts */
    outb(COM1 + 3, 0x80); /* enable DLAB */
    outb(COM1 + 0, 0x03); /* divisor low: 38400 baud */
    outb(COM1 + 1, 0x00); /* divisor high */
    outb(COM1 + 3, 0x03); /* 8N1, DLAB off */
    outb(COM1 + 2, 0xC7); /* enable + clear 14-byte FIFO */
    outb(COM1 + 4, 0x0B); /* IRQs disabled, RTS/DSR set */
}

static int tx_empty(void) {
    return inb(COM1 + 5) & 0x20;
}

void serial_putc(char c) {
    while (!tx_empty()) {
    }
    outb(COM1, (uint8_t)c);
}

void serial_write(const char *str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            serial_putc('\r');
        }
        serial_putc(str[i]);
    }
}

void serial_uint(unsigned int value) {
    char buf[11];
    utoa(value, buf, 10);
    serial_write(buf);
}

void serial_hex(unsigned int value) {
    char buf[9];
    utoa(value, buf, 16);
    serial_write("0x");
    serial_write(buf);
}
