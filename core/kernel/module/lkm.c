#include "lkm.h"
#include "elf_loader.h"
#include <string/string.h>
#include <kernel/mem/phys/physmem.h>
#include <outputs/print.h>
#include <string/string.h>
extern void kfree(void *ptr);
extern void *memset(void *s, int c, size_t n);
static struct lkm_module loaded_lkms[MAX_LKMS];
static int lkm_count = 0;
int lkm_init(void) {
    print("[LKM] ", 0);
    print("init loadable kernel modules system\n", 0);
    memset(loaded_lkms, 0, sizeof(loaded_lkms));
    lkm_count = 0;
    return 0;
}
int lkm_load(const char *path) {
    if (lkm_count >= MAX_LKMS) {
        return -1;
    }
    void *module_base;
    size_t module_size;
    Elf32_Addr entry_point;
    if (elf_load_module(path, &module_base, &module_size, &entry_point) != 0) {
        return -1;
    }
    int (*init_func)(void) = (int (*)(void))entry_point;
    if (init_func() != 0) {
        kfree(module_base);
        return -1;
    }
    struct lkm_module *mod = &loaded_lkms[lkm_count++];
    mod->name = "unknown";
    mod->module_core = module_base;
    mod->size = module_size;
    mod->init = init_func;
    return 0;
}
int lkm_unload(const char *name) {
    for (int i = 0; i < lkm_count; i++) {
        if (str_equals(loaded_lkms[i].name, name)) {
            if (loaded_lkms[i].exit) {
                loaded_lkms[i].exit();
            }
            if (loaded_lkms[i].module_core) {
                kfree(loaded_lkms[i].module_core);
            }
            for (int j = i; j < lkm_count - 1; j++) {
                loaded_lkms[j] = loaded_lkms[j + 1];
            }
            lkm_count--;
            return 0;
        }
    }
    return -1;
}
struct lkm_module *lkm_find(const char *name) {
    for (int i = 0; i < lkm_count; i++) {
        if (str_equals(loaded_lkms[i].name, name)) {
            return &loaded_lkms[i];
        }
    }
    return NULL;
}
int lkm_register_builtin(struct lkm_module *mod) {
    if (lkm_count >= MAX_LKMS || !mod) return -1;
    loaded_lkms[lkm_count++] = *mod;
    return 0;
}