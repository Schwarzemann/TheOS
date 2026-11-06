#ifndef THEOS_BIN_H
#define THEOS_BIN_H

#include <stdint.h>

// Dispatches a single shell input line to the matching handler in kernel/bin/
void run_command(const char *cmd);

// Prints "<op>: <message>\n" for a negative FS_ERR_* code from fs.h.
void print_fs_error(const char *op, int err);

// Finds the file at `path` relative to the current directory, creating it if needed.
// Returns its index, or prints the problem and returns a negative FS_ERR_* code.
int32_t file_for_write(const char *op, const char *path);

#endif
