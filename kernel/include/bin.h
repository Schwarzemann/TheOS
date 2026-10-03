#ifndef THEOS_BIN_H
#define THEOS_BIN_H

// Dispatches a single shell input line to the matching handler in kernel/bin/
void run_command(const char *cmd);

#endif
