// Command table and dispatcher for the shell (kernel/kernel.c).

#include <bin.h>
#include <cmds.h>
#include <fs.h>
#include <string.h>
#include <vga.h>

#define NAME_BUF_SIZE 64

typedef struct {
    const char *name;
    void (*fn)(const char *arg);
} command_t;

static const command_t commands[] = {
    { "help",    cmd_help },
    { "clear",   cmd_clear },
    { "ls",      cmd_ls },
    { "cat",     cmd_cat },
    { "cd",      cmd_cd },
    { "pwd",     cmd_pwd },
    { "memory",  cmd_memory },
    { "disk",    cmd_disk },
    { "uptime",  cmd_uptime },
    { "meminfo", cmd_meminfo },
    { "touch",   cmd_touch },
    { "mkdir",   cmd_mkdir },
    { "rm",      cmd_rm },
    { "rmdir",   cmd_rmdir },
    { "write",   cmd_write },
    { "append",  cmd_append },
    { "mv",      cmd_mv },
    { "cp",      cmd_cp },
};
#define COMMAND_COUNT (sizeof(commands) / sizeof(commands[0]))

void print_fs_error(const char *op, int err) {
    vga_print(op);
    switch (err) {
        case FS_ERR_EXISTS:   vga_print(": already exists\n"); break;
        case FS_ERR_NOTDIR:   vga_print(": not a directory\n"); break;
        case FS_ERR_ISDIR:    vga_print(": is a directory\n"); break;
        case FS_ERR_FULL:     vga_print(": directory is full\n"); break;
        case FS_ERR_NAME:     vga_print(": invalid name\n"); break;
        case FS_ERR_NOSPACE:  vga_print(": disk is full\n"); break;
        case FS_ERR_NOTEMPTY: vga_print(": directory is not empty\n"); break;
        case FS_ERR_CYCLE:    vga_print(": cannot move a directory into itself\n"); break;
        default:              vga_print(": error\n"); break;
    }
}

void run_command(const char *cmd) {
    if (cmd[0] == '\0') {
        return;
    }

    char name[NAME_BUF_SIZE];
    const char *arg = "";
    size_t i = 0;
    while (cmd[i] != '\0' && cmd[i] != ' ') {
        name[i] = cmd[i];
        i++;
    }
    name[i] = '\0';
    if (cmd[i] == ' ') {
        arg = cmd + i + 1;
    }

    for (size_t c = 0; c < COMMAND_COUNT; c++) {
        if (strcmp(name, commands[c].name) == 0) {
            commands[c].fn(arg);
            return;
        }
    }

    vga_print("Unknown command: ");
    vga_print(cmd);
    vga_putc('\n');
}
