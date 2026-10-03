#ifndef THEOS_CWD_H
#define THEOS_CWD_H

#include <stddef.h>
#include <stdint.h>

/* Tracks the shell's current directory within the TOFS tree mounted
 * by kernel/fs.c, for the cd/pwd/ls/cat commands and the prompt. */

void cwd_init(void);

/* Changes directory (absolute, relative, "." or ".."). Returns 0 and keeps the
 * current directory if the path is missing or not a directory. */
int cwd_chdir(const char *path);

/* The current directory, as an fs.h index (or FS_ROOT). */
int32_t cwd_get(void);

/* Writes the absolute path of the current directory (e.g. "/" or
 * "/bin") into buf. */
void cwd_path(char *buf, size_t buf_size);

#endif
