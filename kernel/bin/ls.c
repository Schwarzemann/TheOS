#include <cmds.h>
#include <vga.h>
#include <fs.h>
#include <cwd.h>

static void print_entry(const fs_entry_t *e) {
    vga_print(e->name);
    if (e->type == FS_TYPE_DIR) {
        vga_print("/\n");
    } else {
        vga_print("  (");
        vga_print_uint(e->size_bytes);
        vga_print(" bytes)\n");
    }
}

void cmd_ls(const char *arg) {
    int32_t dir = cwd_get();
    if (arg[0] != '\0') {
        if (!fs_resolve(cwd_get(), arg, &dir)) {
            vga_print("ls: no such file or directory: ");
            vga_print(arg);
            vga_putc('\n');
            return;
        }
        if (!fs_is_dir(dir)) {
            print_entry(fs_entry_at((uint32_t)dir));
            return;
        }
    }

    uint32_t count = fs_entry_count();
    for (uint32_t i = 0; i < count; i++) {
        const fs_entry_t *e = fs_entry_at(i);
        if (e->parent == dir && e->type != FS_TYPE_FREE) {
            print_entry(e);
        }
    }
}
