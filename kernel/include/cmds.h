#ifndef THEOS_CMDS_H
#define THEOS_CMDS_H


void cmd_help(const char *arg);
void cmd_clear(const char *arg);
void cmd_ls(const char *arg);
void cmd_cat(const char *arg);
void cmd_cd(const char *arg);
void cmd_pwd(const char *arg);
void cmd_memory(const char *arg);
void cmd_disk(const char *arg);
void cmd_uptime(const char *arg);
void cmd_meminfo(const char *arg);
void cmd_touch(const char *arg);
void cmd_mkdir(const char *arg);
void cmd_rm(const char *arg);
void cmd_rmdir(const char *arg);
void cmd_write(const char *arg);
void cmd_append(const char *arg);
void cmd_mv(const char *arg);
void cmd_cp(const char *arg);

#endif
