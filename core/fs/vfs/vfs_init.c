#include "init.h"
char *logpath = "/tmp/log";
int init_boot_log = -1;
#include "vfs.h"
#include <kernel/mem/kernel_memory/kernel_memory.h>
#include <kernel/include/reqs.h>
#include <kernel/module/module.h>
#include <kernel/exceptions/panic.h>
#include <string/string.h>
#include <kernel/graph/theme.h>
#include <config/boot.h>
#include "ext4/ext4.h"
static void load_limine_module(void) {
    if (!module_request.response || module_request.response->module_count == 0) {
        BOOTUP_PRINT("[FS] ", gray_70);
        BOOTUP_PRINT("no limine modules to copy\n", theme_white());
        return;
    }
    struct limine_module_response *response = (struct limine_module_response *)module_request.response;
    BOOTUP_PRINT("[FS] ", gray_70);
    BOOTUP_PRINT("setting UI config...\n", theme_white());
    fs_mkdir("/boot/ui");
    fs_mkdir("/boot/ui/assets");
    BOOTUP_PRINT("[FS] ", gray_70);
    BOOTUP_PRINT("copying limine modules to VFS:\n", theme_white());
    for (u64 i = 0; i < response->module_count; i++) {
        char buf[32];
        struct limine_file *module = response->modules[i];
        const char *filename = module->path;
        const char *last_slash = filename;
        for (const char *p = filename; *p; p++) {
            if (*p == '/') last_slash = p + 1;
        }
        filename = last_slash;
        char vfs_path[256];
        str_copy(vfs_path, "/boot/ui/assets/");
        str_append(vfs_path, filename);
        int fd = fs_open(vfs_path, O_CREAT | O_WRONLY);
        if (fd < 0) {
            BOOTUP_PRINT("  ERROR: cannot create ", theme_red());
            BOOTUP_PRINT(vfs_path, theme_white());
            BOOTUP_PRINT("\n", theme_white());
            continue;
        }
        ssize_t written = fs_write(fd, (void*)module->address, module->size);
        fs_close(fd);
        if (written > 0) {
            BOOTUP_PRINT("  ", gray_70);
            BOOTUP_PRINT(module->path, theme_white());
            BOOTUP_PRINT(" : ", theme_white());
            BOOTUP_PRINT(vfs_path, theme_white());
            str_copy(buf, " with ");
            str_append_uint(buf, (u32)written);
            str_append(buf, " bytes\n");
            BOOTUP_PRINT(buf,  theme_white());
        }
    }
}
void fs_system_init(void *kernel_memory)
{
    fs_kernel_memory = kernel_memory;
    fs_init();
    tmpfs_register();
    devfs_register();
    ext4_register();
    BOOTUP_PRINT("[FS] ", gray_70);
    BOOTUP_PRINT("mounting roots: \n", theme_white());
    fs_mount(NULL, ROOT_MOUNT_DEFAULT, ROOTFS);
    fs_mkdir(DEV_DIRECTORY);
    fs_mkdir(TMP_DIRECTORY);
    fs_mkdir(BOOT_DIRECTORY);
    fs_mkdir("/mnt");
    fs_mkdir("/home");
    fs_mkdir("/etc");
    fs_mkdir("/bin");
    fs_mkdir("/usr");
    fs_mkdir("/usr/bin");
    fs_mkdir("/usr/lib");
    fs_mkdir("/var");
    fs_mkdir("/var/log");
    fs_mkdir("/proc");
    fs_mkdir("/sys");
    fs_mkdir("/root");
    fs_mkdir("/opt");
    init_boot_log = fs_open(logpath, O_CREAT | O_WRONLY);
    BOOTUP_PRINTF("[FS] wrote %s \n", logpath);
    if (init_boot_log < 0) {
        panic("Cannot open [logs]");
    }
    fs_mount(NULL, DEV_MOUNT_DEFAULT, DEVFS);
    load_limine_module();
}
void fs_register_mods()
{
    int total = module_get_count();
    int dev_cnt = 0;
    BOOTUP_PRINT("[FS] ", gray_70);
    BOOTUP_PRINT("scann modules:\n", theme_white());
    for (int i = 0; i < total; i++) {
        driver_module *mod = module_get_by_index(i);
        if (mod && mod->mount && str_starts_with(mod->mount, _DEV)) {
            if (devfs_register_device(mod) == 0) {
                dev_cnt++;
            }
        }
    }
    BOOTUP_PRINT("[FS] ", gray_70);
    BOOTUP_PRINT("registered ", theme_white());
    BOOTUP_PRINT_INT(dev_cnt, theme_white());
    BOOTUP_PRINT(" device(s)\n", theme_white());
}
void fs_create_test_file(void) {
    int fd = fs_open("/tmp/t", O_CREAT | O_WRONLY);
    if (fd < 0) {
        BOOTUP_PRINT("[FS] ", gray_70);
        BOOTUP_PRINT("failed to create file\n", theme_white());
        return;
    }
    fs_write(fd, "this is a test for fs this file has no other use", str_len("this is a test for fs this file has no other use"));
    fs_close(fd);
    BOOTUP_PRINT("[FS] ", gray_70);
    BOOTUP_PRINT("created ", theme_white());
    BOOTUP_PRINT("/tmp/t", theme_white());
    BOOTUP_PRINT("\n", theme_white());
}