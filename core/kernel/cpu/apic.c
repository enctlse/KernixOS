#include "apic.h"
#include <kernel/include/ports.h>
#include <kernel/include/reqs.h>
#include <kernel/cpu/cpu.h>
#include <kernel/mem/paging/paging.h>
#include <theme/stdclrs.h>
#include <kernel/graph/theme.h>
#include <theme/tmx.h>
#include <memory/main.h>
static volatile u32* apic_base = NULL;
static int x2apic = 0;
void apic_init(void) {
    BOOTUP_PRINT("[APIC] Starting apic_init()\n", GFX_GRAY_70);
    BOOTUP_PRINT("[APIC] Checking CPU APIC feature\n", GFX_GRAY_70);
    if (!cpu_has_feature(CPU_FEATURE_APIC)) {
        BOOTUP_PRINT("[APIC] APIC not supported by CPU\n", GFX_RED);
        return;
    }
    BOOTUP_PRINT("[APIC] APIC supported\n", GFX_GRAY_70);
    BOOTUP_PRINT("[APIC] Checking CPU MSR feature\n", GFX_GRAY_70);
    if (!cpu_has_feature(CPU_FEATURE_MSR)) {
        BOOTUP_PRINT("[APIC] MSR not supported by CPU\n", GFX_RED);
        return;
    }
    BOOTUP_PRINT("[APIC] MSR supported\n", GFX_GRAY_70);
    BOOTUP_PRINT("[APIC] Checking hhdm_request.response\n", GFX_GRAY_70);
    if (!hhdm_request.response) {
        BOOTUP_PRINT("[APIC] hhdm_request.response is NULL\n", GFX_RED);
        return;
    }
    BOOTUP_PRINT("[APIC] hhdm_request.response valid\n", GFX_GRAY_70);
    BOOTUP_PRINT("[APIC] About to read APIC base MSR\n", GFX_GRAY_70);
    u32 low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(APIC_BASE_MSR));
    BOOTUP_PRINT("[APIC] MSR read successful, low=0x", GFX_GRAY_70);
    BOOTUP_PRINT_INT(low, GFX_CYAN);
    BOOTUP_PRINT(" high=0x", GFX_GRAY_70);
    BOOTUP_PRINT_INT(high, GFX_CYAN);
    BOOTUP_PRINT("\n", GFX_GRAY_70);
    u64 base_addr = ((u64)high << 32) | low;
    base_addr &= ~0xFFF;
    if (cpu_has_feature(CPU_FEATURE_X2APIC)) {
        if (low & (1 << 10)) {
            x2apic = 1;
        } else {
            low |= (1 << 10);
            x2apic = 1;
        }
    } else {
        x2apic = 0;
    }
    BOOTUP_PRINT("[APIC] APIC base address calculated: 0x", GFX_GRAY_70);
    BOOTUP_PRINT_INT(base_addr >> 32, GFX_CYAN);
    BOOTUP_PRINT_INT(base_addr & 0xFFFFFFFF, GFX_CYAN);
    BOOTUP_PRINT(", x2APIC: ", GFX_GRAY_70);
    BOOTUP_PRINT_INT(x2apic, GFX_CYAN);
    BOOTUP_PRINT("\n", GFX_GRAY_70);
    BOOTUP_PRINT("[APIC] APIC base address validation skipped (always valid for mapping)\n", GFX_GRAY_70);
    if (base_addr == 0) {
        BOOTUP_PRINT("[APIC] ERROR: APIC base address is 0, cannot proceed\n", GFX_RED);
        return;
    }
    BOOTUP_PRINT("[APIC] About to calculate apic_base\n", GFX_GRAY_70);
    apic_base = (volatile u32*)(base_addr + hhdm_request.response->offset);
    BOOTUP_PRINT("[APIC] apic_base calculated: 0x", GFX_GRAY_70);
    BOOTUP_PRINT_INT((u64)apic_base >> 32, GFX_CYAN);
    BOOTUP_PRINT_INT((u64)apic_base & 0xFFFFFFFF, GFX_CYAN);
    BOOTUP_PRINT("\n", GFX_GRAY_70);
    paging_map_page(hhdm_request.response, (u64)apic_base, base_addr, PTE_PRESENT | PTE_WRITABLE | PTE_PCD);
    low |= APIC_BASE_MSR_ENABLE;
    BOOTUP_PRINT("[APIC] About to enable APIC via MSR write\n", GFX_GRAY_70);
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(APIC_BASE_MSR));
    BOOTUP_PRINT("[APIC] MSR write successful\n", GFX_GRAY_70);
    BOOTUP_PRINT("[APIC] About to write spurious vector\n", GFX_GRAY_70);
    apic_write_reg(APIC_SPURIOUS, 0x1FF);
    BOOTUP_PRINT("[APIC] Spurious vector write done\n", GFX_GRAY_70);
    BOOTUP_PRINT("[APIC] About to read spurious reg\n", GFX_GRAY_70);
    u32 spurious = apic_read_reg(APIC_SPURIOUS);
    BOOTUP_PRINT("[APIC] Read spurious reg done, value=0x", GFX_GRAY_70);
    BOOTUP_PRINT_INT(spurious, GFX_CYAN);
    BOOTUP_PRINT("\n", GFX_GRAY_70);
    BOOTUP_PRINT("[APIC] About to enable spurious vector\n", GFX_GRAY_70);
    apic_write_reg(APIC_SPURIOUS, spurious | 0x100);
    BOOTUP_PRINT("[APIC] Spurious vector enabled\n", GFX_GRAY_70);
    BOOTUP_PRINT("[APIC] APIC initialized successfully\n", GFX_GRAY_70);
}
u32 apic_get_id(void) {
    if (x2apic) {
        u32 low, high;
        __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0x802));
        return low;
    } else {
        if (!apic_base) return 0;
        return apic_read_reg(APIC_ID) >> 24;
    }
}
void apic_send_ipi(u32 apic_id, u32 vector) {
    if (!apic_base) return;
    apic_write_reg(APIC_ICR_HIGH, apic_id << 24);
    apic_write_reg(APIC_ICR_LOW, vector);
}
void apic_send_init(u32 apic_id) {
    if (!apic_base) return;
    apic_write_reg(APIC_ICR_HIGH, apic_id << 24);
    apic_write_reg(APIC_ICR_LOW, 0xC500);
}
void apic_send_sipi(u32 apic_id, u32 vector) {
    if (!apic_base) return;
    apic_write_reg(APIC_ICR_HIGH, apic_id << 24);
    apic_write_reg(APIC_ICR_LOW, 0x600 | (vector >> 12));
}
void apic_eoi(void) {
    if (!apic_base) return;
    apic_write_reg(APIC_EOI, 0);
}
u32 apic_read_reg(u32 reg) {
    if (!apic_base) return 0;
    return apic_base[reg / 4];
}
void apic_write_reg(u32 reg, u32 value) {
    if (!apic_base) return;
    apic_base[reg / 4] = value;
}