#ifndef APIC_H
#define APIC_H
#include <types.h>
#define APIC_BASE_MSR 0x1B
#define APIC_BASE_MSR_ENABLE 0x800
#define APIC_ID 0x20
#define APIC_VERSION 0x30
#define APIC_TPR 0x80
#define APIC_APR 0x90
#define APIC_PPR 0xA0
#define APIC_EOI 0xB0
#define APIC_RRD 0xC0
#define APIC_LDR 0xD0
#define APIC_DFR 0xE0
#define APIC_SPURIOUS 0xF0
#define APIC_ISR_BASE 0x100
#define APIC_TMR_BASE 0x180
#define APIC_IRR_BASE 0x200
#define APIC_ESR 0x280
#define APIC_ICR_LOW 0x300
#define APIC_ICR_HIGH 0x310
#define APIC_LVT_TIMER 0x320
#define APIC_LVT_THERMAL 0x330
#define APIC_LVT_PERF 0x340
#define APIC_LVT_LINT0 0x350
#define APIC_LVT_LINT1 0x360
#define APIC_LVT_ERROR 0x370
#define APIC_TIMER_INIT 0x380
#define APIC_TIMER_CURRENT 0x390
#define APIC_TIMER_DIV 0x3E0
void apic_init(void);
u32 apic_get_id(void);
void apic_send_ipi(u32 apic_id, u32 vector);
void apic_send_init(u32 apic_id);
void apic_send_sipi(u32 apic_id, u32 vector);
void apic_eoi(void);
u32 apic_read_reg(u32 reg);
void apic_write_reg(u32 reg, u32 value);
#endif