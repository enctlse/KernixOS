#include "vfs.h"
#include <drivers/memory/mem.h>
#include <string/string.h>
#include <kernel/mem/kernel_memory/kernel_memory.h>
#include <config/boot.h>
int fs_open(const char *path, int flags) {
    fs_node *node = fs_resolve(path);
    if (!node && (flags & O_CREAT)) {
        char ppath[FS_MAX_PATH];
        char fname[64];
        const char *last = path;
        for (const char *p = path; *p; p++) {
            if (*p == '/') last = p;
        }
        int plen = last - path;
        if (plen == 0) plen = 1;
        for (int i = 0; i < plen && i < FS_MAX_PATH - 1; i++) {
            ppath[i] = path[i];
        }
        ppath[plen] = '\0';
        str_copy(fname, last + 1);
        fs_node *parent = fs_resolve(ppath);
        if (parent && parent->ops && parent->ops->create) {
            parent->ops->create(parent, fname);
            node = fs_resolve(path);
        }
    }
    if (!node) return -1;
    fs_file *file = (fs_file*)kernel_memory_create((kernel_memory_t*)fs_kernel_memory, sizeof(fs_file));
    if (!file) return -1;
    file->node = node;
    file->pos = 0;
    file->flags = flags;
    if (node->ops && node->ops->open) {
        if (node->ops->open(node, file) != 0) {
            kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)file);
            return -1;
        }
    }
    int fd = fs_alloc_fd(file);
    if (fd < 0) {
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)file);
        return -1;
    }
    return fd;
}
int fs_close(int fd) {
    fs_file *file = fs_get_file(fd);
    if (!file) return -1;
    if (file->node->ops && file->node->ops->close) {
        file->node->ops->close(file);
    }
    kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)file);
    fs_free_fd(fd);
    return 0;
}
ssize_t fs_read(int fd, void *buf, size_t cnt) {
    fs_file *file = fs_get_file(fd);
    if (!file) return -1;
    if (!file->node->ops || !file->node->ops->read) return -1;
    return file->node->ops->read(file, buf, cnt);
}
ssize_t fs_write(int fd, const void *buf, size_t cnt) {
    fs_file *file = fs_get_file(fd);
    if (!file) return -1;
    if (!file->node->ops || !file->node->ops->write) return -1;
    return file->node->ops->write(file, buf, cnt);
}
int fs_mkdir(const char *path) {
    char ppath[FS_MAX_PATH];
    char dname[64];
    const char *last = path;
    for (const char *p = path; *p; p++) {
        if (*p == '/') last = p;
    }
    int plen = last - path;
    if (plen == 0) plen = 1;
    for (int i = 0; i < plen && i < FS_MAX_PATH - 1; i++) {
        ppath[i] = path[i];
    }
    ppath[plen] = '\0';
    str_copy(dname, last + 1);
    fs_node *parent = fs_resolve(ppath);
    if (!parent || !parent->ops || !parent->ops->mkdir) return -1;
    int ret = parent->ops->mkdir(parent, dname);
    if (ret == 0) {
        SYSTEM_PRINT("     ", gray_70);
        SYSTEM_PRINT("mkdir ", st_white);
        SYSTEM_PRINT(path, st_cyan);
        SYSTEM_PRINT("\n", st_white);
    }
    return ret;
}