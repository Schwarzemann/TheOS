#include <cmds.h>
#include <vga.h>
#include <fs.h>
#include <cwd.h>

void cmd_ls(const char *arg) {
    (void)arg;
    int32_t dir = cwd_get();
    uint32_t count = fs_entry_count();
    for (uint32_t i = 0; i < count; i++) {
        const fs_entry_t *e = fs_entry_at(i);
        if (e->parent != dir) {
            continue;
        }
        vga_print(e->name);
        if (e->type == FS_TYPE_DIR) {
            vga_print("/\n");
        } else {
            vga_print("  (");
            vga_print_uint(e->size_bytes);
            vga_print(" bytes)\n");
        }
    }
}
