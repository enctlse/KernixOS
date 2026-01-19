#include "defs.h"
volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};
volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};
volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};
volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0
};
volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST_ID,
    .revision = 0
};
volatile struct limine_mp_request mp_request = {
    .id = LIMINE_MP_REQUEST_ID,
    .revision = 0
};
extern struct kernel_symbol __start_export[];
extern struct kernel_symbol __stop_export[];
void *kernel_symbol_lookup(const char *name) {
    struct kernel_symbol *sym;
    for (sym = __start_export; sym < __stop_export; sym++) {
        if (str_equals(sym->name, name)) {
            return sym->addr;
        }
    }
    return NULL;
}