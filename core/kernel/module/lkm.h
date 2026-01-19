#ifndef LKM_H
#define LKM_H
#include <outputs/types.h>
#define MAX_LKMS 256
struct lkm_module {
    const char *name;
    const char *version;
    const char **depends;
    int (*init)(void);
    void (*exit)(void);
    void *module_core;
    size_t size;
};
#define MODULE_INIT(func) \
    int init_module(void) { return func(); }
#define MODULE_EXIT(func) \
    void cleanup_module(void) { func(); }
#define MODULE_LICENSE(lic) \
    static const char __module_license[] = lic;
#define MODULE_AUTHOR(auth) \
    static const char __module_author[] = auth;
#define MODULE_DESCRIPTION(desc) \
    static const char __module_description[] = desc;
int lkm_load(const char *path);
int lkm_unload(const char *name);
struct lkm_module *lkm_find(const char *name);
int lkm_init(void);
int lkm_register_builtin(struct lkm_module *mod);
#endif