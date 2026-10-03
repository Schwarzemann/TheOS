#include <cmds.h>
#include <vga.h>
#include <fs.h>
#include <cwd.h>
#include <heap.h>

void cmd_cat(const char *arg) {
    if (arg[0] == '\0') {
        vga_print("usage: cat <file>\n");
        return;
    }

    int32_t index;
    if (!fs_resolve(cwd_get(), arg, &index) || fs_is_dir(index)) {
        vga_print("cat: no such file: ");
        vga_print(arg);
        vga_putc('\n');
        return;
    }

    const fs_entry_t *e = fs_entry_at((uint32_t)index);
    char *buf = kmalloc(fs_sectors(e) * 512);
    if (buf == 0) {
        vga_print("cat: out of memory\n");
        return;
    }
    uint32_t size = fs_read(e, buf);
    vga_print_n(buf, size);
    if (size == 0 || buf[size - 1] != '\n') {
        vga_putc('\n');
    }
    kfree(buf);
}
