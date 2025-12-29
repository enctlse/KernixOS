#include "apic.h"
#include <kernel/include/ports.h>
static volatile u32* apic_base = NULL;
void apic_init(void) {
    u32 low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(APIC_BASE_MSR));
    u64 base_addr = ((u64)high << 32) | low;
    base_addr &= ~0xFFF;
    apic_base = (volatile u32*)base_addr;
    low |= APIC_BASE_MSR_ENABLE;
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(APIC_BASE_MSR));
    apic_write_reg(APIC_SPURIOUS, 0x1FF);
    u32 spurious = apic_read_reg(APIC_SPURIOUS);
    apic_write_reg(APIC_SPURIOUS, spurious | 0x100);
}
u32 apic_get_id(void) {
    if (!apic_base) return 0;
    return apic_read_reg(APIC_ID) >> 24;
}
void apic_send_ipi(u32 apic_id, u32 vector) {
    if (!apic_base) return;
    apic_write_reg(APIC_ICR_HIGH, apic_id << 24);
    apic_write_reg(APIC_ICR_LOW, vector);
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