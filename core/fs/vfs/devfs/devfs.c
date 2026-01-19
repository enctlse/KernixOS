#include "../vfs.h"
#include <drivers/memory/mem.h>
#include <string/string.h>
#include <kernel/mem/kernel_memory/kernel_memory.h>
#include <ui/theme/colors.h>
#include <config/boot.h>
static fs_node *devfs_root = NULL;
static int devfs_open(fs_node *node, fs_file *file) {
    (void)file;
    devfs_data *dev = (devfs_data*)node->priv;
    if (!dev || !dev->handler) return -1;
    if (dev->handler->access) {
        dev->handle = dev->handler->access(node->name);
        if (!dev->handle) return -1;
    }
    return 0;
}
static int devfs_close(fs_file *file) {
    devfs_data *dev = (devfs_data*)file->node->priv;
    if (!dev) return -1;
    dev->handle = NULL;
    return 0;
}
static ssize_t devfs_read(fs_file *file, void *buf, size_t cnt) {
    devfs_data *dev = (devfs_data*)file->node->priv;
    if (!dev || !dev->handler || !dev->handler->retrieve) return -1;
    return dev->handler->retrieve(dev->handle, buf, cnt);
}
static ssize_t devfs_write(fs_file *file, const void *buf, size_t cnt) {
    devfs_data *dev = (devfs_data*)file->node->priv;
    if (!dev || !dev->handler || !dev->handler->store) return -1;
    return dev->handler->store(dev->handle, buf, cnt);
}
static fs_node* devfs_lookup(fs_node *dir, const char *name) {
    if (dir->type != FS_DIR) return NULL;
    fs_node *child = dir->children;
    while (child) {
        if (str_equals(child->name, name)) {
            return child;
        }
        child = child->next;
    }
    return NULL;
}
static fs_ops devfs_ops = {
    .open = devfs_open,
    .close = devfs_close,
    .read = devfs_read,
    .write = devfs_write,
    .lookup = devfs_lookup,
    .create = NULL,
    .mkdir = NULL,
};
static int devfs_mount(const char *src, const char *tgt, fs_mnt *mnt) {
    {
        (void)src;(void)tgt;
    }
    fs_node *root = fs_mknode("dev", FS_DIR);
    if (!root) return -1;
    root->ops = &devfs_ops;
    mnt->root = root;
    devfs_root = root;
    return 0;
}
static fs_type devfs = {
    .name = "devfs",
    .mount = devfs_mount,
    .ops = &devfs_ops,
};
void devfs_register(void) {
    fs_register(&devfs);
}
int devfs_register_device(struct component_handler *handler)
{
    if (!handler || !devfs_root) return -1;
    const char *path = handler->attachment_point;
    const char *name = path;
    if (str_starts_with(path, "/dev/")) {
        name = path + 5;
    }
    if (devfs_lookup(devfs_root, name)) {
        return -1;
    }
    fs_node *node = fs_mknode(name, FS_DEV);
    if (!node) return -1;
    devfs_data *data = (devfs_data*)kernel_memory_create((kernel_memory_t*)fs_kernel_memory, sizeof(devfs_data));
    if (!data) {
        kernel_memory_free((kernel_memory_t*)fs_kernel_memory, (u64*)node);
        return -1;
    }
    data->handler = handler;
    data->handle = NULL;
    node->priv = data;
    node->ops = &devfs_ops;
    SYSTEM_PRINT("[DevFS] ", gray_70);
    SYSTEM_PRINT("registered ", st_white);
    SYSTEM_PRINT(name, st_cyan);
    SYSTEM_PRINT("\n", st_white);
    return 0;
}