#include <cwd.h>
#include <fs.h>
#include <string.h>

static int32_t cwd_index = FS_ROOT;

void cwd_init(void) {
    cwd_index = FS_ROOT;
}

int cwd_chdir(const char *path) {
    int32_t resolved;
    if (!fs_resolve(cwd_index, path, &resolved) || !fs_is_dir(resolved)) {
        return 0;
    }
    cwd_index = resolved;
    return 1;
}

int32_t cwd_get(void) {
    return cwd_index;
}

void cwd_path(char *buf, size_t buf_size) {
    const char *parts[FS_MAX_ENTRIES];
    int depth = 0;
    int32_t idx = cwd_index;
    while (idx != FS_ROOT && depth < FS_MAX_ENTRIES) {
        parts[depth++] = fs_entry_at((uint32_t)idx)->name;
        idx = fs_entry_at((uint32_t)idx)->parent;
    }

    size_t pos = 0;
    for (int i = depth - 1; i >= 0; i--) {
        if (pos + 1 < buf_size) {
            buf[pos++] = '/';
        }
        size_t len = strlen(parts[i]);
        if (pos + len < buf_size) {
            memcpy(buf + pos, parts[i], len);
            pos += len;
        }
    }
    if (pos == 0 && buf_size > 1) {
        buf[pos++] = '/';
    }
    buf[pos < buf_size ? pos : buf_size - 1] = '\0';
}
