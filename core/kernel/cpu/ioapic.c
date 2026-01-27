#include "ioapic.h"
#include <kernel/include/io.h>
#include <kernel/mem/paging/paging.h>
#include <ui/theme/colors.h>
#include <config/boot.h>
extern struct limine_hhdm_request hhdm_request;
static volatile u32* ioapic_base = NULL;
void ioapic_init(u32 address) {
    SYSTEM_PRINT("[IOAPIC] Initializing IOAPIC at 0x", gray_70);
    SYSTEM_PRINT_INT(address, cyan);
    SYSTEM_PRINT("\n", gray_70);
    ioapic_base = (volatile u32*)(address + hhdm_request.response->offset);
    paging_map_page(hhdm_request.response, (u64)ioapic_base, address, PTE_PRESENT | PTE_WRITABLE | PTE_PCD);
    u32 ver = ioapic_read(IOAPIC_VER);
    SYSTEM_PRINT("[IOAPIC] Version: ", gray_70);
    SYSTEM_PRINT_INT(ver, cyan);
    SYSTEM_PRINT("\n", gray_70);
}
void ioapic_write(u32 reg, u32 value) {
    if (!ioapic_base) return;
    ioapic_base[IOAPIC_IOREGSEL / 4] = reg;
    ioapic_base[IOAPIC_IOREGWIN / 4] = value;
}
u32 ioapic_read(u32 reg) {
    if (!ioapic_base) return 0;
    ioapic_base[IOAPIC_IOREGSEL / 4] = reg;
    return ioapic_base[IOAPIC_IOREGWIN / 4];
}
void ioapic_set_redirect(u32 irq, u32 apic_id, u32 vector, u16 flags) {
    u32 low = vector | (1 << 16);
    if ((flags & 0xC) == 0x4) low |= (1 << 13);
    if ((flags & 0x3) == 0x1) low |= (1 << 15);
    u32 high = apic_id << 24;
    ioapic_write(IOAPIC_REDIRECT_BASE + irq * 2, low);
    ioapic_write(IOAPIC_REDIRECT_BASE + irq * 2 + 1, high);
    low &= ~(1 << 16);
    ioapic_write(IOAPIC_REDIRECT_BASE + irq * 2, low);
}