#ifndef SYSTEM_DEFS_H
#define SYSTEM_DEFS_H

#include <limine/limine.h>
#include <outputs/types.h>
#include <string/string.h>

// Limine requests
extern volatile struct limine_framebuffer_request framebuffer_request;
extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;
extern volatile struct limine_module_request module_request;
extern volatile struct limine_rsdp_request rsdp_request;
extern volatile struct limine_mp_request mp_request;

// Kernel symbol structure
struct kernel_symbol {
    const char *name;
    void *addr;
};

// Export macro
#define EXPORT_SYMBOL(sym) \
    static const char __export_str_##sym[] = #sym; \
    static const struct kernel_symbol __export_##sym \
    __attribute__((section(".export"))) = { __export_str_##sym, &sym };

// Function to lookup symbol
void *kernel_symbol_lookup(const char *name);

#endif