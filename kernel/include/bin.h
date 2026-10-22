#ifndef THEOS_BIN_H
#define THEOS_BIN_H

// Dispatches a single shell input line to the matching handler in kernel/bin/
void run_command(const char *cmd);

// Prints "<op>: <message>\n" for a negative FS_ERR_* code from fs.h.
void print_fs_error(const char *op, int err);

#endif
