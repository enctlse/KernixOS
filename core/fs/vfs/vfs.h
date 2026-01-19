#ifndef _VFS_H
#define _VFS_H
#include <outputs/types.h>
#define FS_FILE 0x01
#define FS_DIR  0x02
#define FS_DEV  0x04
#define O_RDONLY 0x01
#define O_WRONLY 0x02
#define O_RDWR   0x03
#define O_CREAT  0x04
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
typedef long off_t;
#define FS_MAX_MNTS 20
#define FS_MAX_FDS  64
#define FS_MAX_PATH 200
typedef struct fs_node fs_node;
typedef struct fs_file fs_file;
typedef struct fs_mnt  fs_mnt;
typedef struct fs_dirent fs_dirent;
typedef struct fs_ops  fs_ops;
typedef struct fs_type fs_type;
struct fs_dirent {
    char name[256];
    u32 inode;
};
struct fs_node {
    char name[64];
    u8 type;
    u64 size;
    u64 inode;
    fs_ops *ops;
    void *priv;
    fs_node *children;
    fs_node *next;
    fs_node *parent;
};
struct fs_file {
    fs_node *node;
    u64 pos;
    u32 flags;
};
struct fs_ops {
    int (*open)(fs_node *node, fs_file *file);
    int (*close)(fs_file *file);
    ssize_t (*read)(fs_file *file, void *buf, size_t cnt);
    ssize_t (*write)(fs_file *file, const void *buf, size_t cnt);
    fs_node* (*lookup)(fs_node *dir, const char *name);
    int (*create)(fs_node *dir, const char *name);
    int (*mkdir)(fs_node *dir, const char *name);
};
struct fs_type {
    const char *name;
    int (*mount)(const char *src, const char *tgt, fs_mnt *mnt);
    fs_ops *ops;
};
struct fs_mnt {
    char path[FS_MAX_PATH];
    fs_type *type;
    fs_node *root;
    void *priv;
    int active;
};
void fs_init(void);
void fs_system_init(void *kernel_memory);
void fs_register_mods(void);
void fs_create_test_file(void);
int fs_register(fs_type *type);
int fs_mount(const char *src, const char *tgt, const char *type);
fs_node* fs_resolve(const char *path);
fs_node* fs_mknode(const char *name, u8 type);
int fs_addchild(fs_node *parent, fs_node *child);
int fs_alloc_fd(fs_file *file);
void fs_free_fd(int fd);
fs_file* fs_get_file(int fd);
int fs_open(const char *path, int flags);
int fs_close(int fd);
ssize_t fs_read(int fd, void *buf, size_t cnt);
ssize_t fs_write(int fd, const void *buf, size_t cnt);
off_t fs_lseek(int fd, off_t offset, int whence);
int fs_mkdir(const char *path);
extern void *fs_kernel_memory;
typedef struct {
    void *data;
    u64 cap;
} tmpfs_data;
void tmpfs_register(void);
#include <kernel/module/module.h>
typedef struct {
    struct component_handler *handler;
    void *handle;
} devfs_data;
void devfs_register(void);
int devfs_register_device(struct component_handler *handler);
#endif