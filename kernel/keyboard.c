#include <keyboard.h>
#include <isr.h>
#include <io.h>

#define KBD_DATA_PORT   0x60

#define SC_ENTER     0x1C
#define SC_BACKSPACE 0x0E
#define SC_LSHIFT    0x2A
#define SC_RSHIFT    0x36

#define KBD_BUF_SIZE 256

static volatile char buf[KBD_BUF_SIZE];
static volatile unsigned int head, tail;
static int shift_held;

/* US QWERTY scan set 1, make codes only. */
static const char unshifted_map[] = {
    0,   0,   '1', '2', '3', '4', '5', '6',  /* 0x00-0x07 */
    '7', '8', '9', '0', '-', '=', 0,   0,    /* 0x08-0x0F */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',  /* 0x10-0x17 */
    'o', 'p', '[', ']', 0,   0,   'a', 's',  /* 0x18-0x1F */
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  /* 0x20-0x27 */
    '\'','`', 0,  '\\','z', 'x', 'c', 'v',   /* 0x28-0x2F */
    'b', 'n', 'm', ',', '.', '/', 0,   '*',  /* 0x30-0x37 */
    0,   ' ',                                /* 0x38-0x39 */
};

static const char shifted_map[] = {
    0,   0,   '!', '@', '#', '$', '%', '^',
    '&', '*', '(', ')', '_', '+', 0,   0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{', '}', 0,   0,   'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    '"', '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0,   '*',
    0,   ' ',
};

static void push(char c) {
    unsigned int next = (head + 1) % KBD_BUF_SIZE;
    if (next == tail) {
        return; /* buffer full, drop */
    }
    buf[head] = c;
    head = next;
}

static void kbd_irq(registers_t *regs) {
    (void)regs;
    unsigned char scancode = inb(KBD_DATA_PORT);

    if (scancode == SC_LSHIFT || scancode == SC_RSHIFT) {
        shift_held = 1;
        return;
    }
    if (scancode == (0x80 | SC_LSHIFT) || scancode == (0x80 | SC_RSHIFT)) {
        shift_held = 0;
        return;
    }
    if (scancode & 0x80) {
        return; /* other key release */
    }
    if (scancode == SC_ENTER) {
        push('\n');
        return;
    }
    if (scancode == SC_BACKSPACE) {
        push('\b');
        return;
    }
    if (scancode < sizeof(unshifted_map)) {
        char c = shift_held ? shifted_map[scancode] : unshifted_map[scancode];
        if (c != 0) {
            push(c);
        }
    }
}

void kbd_init(void) {
    irq_register(1, kbd_irq);
}

char kbd_getc(void) {
    while (head == tail) {
        __asm__ volatile ("sti; hlt");
    }
    char c = buf[tail];
    tail = (tail + 1) % KBD_BUF_SIZE;
    return c;
}
