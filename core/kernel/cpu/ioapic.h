#ifndef IOAPIC_H
#define IOAPIC_H
#include <outputs/types.h>
#define IOAPIC_IOREGSEL 0x00
#define IOAPIC_IOREGWIN 0x10
#define IOAPIC_ID 0x00
#define IOAPIC_VER 0x01
#define IOAPIC_ARB 0x02
#define IOAPIC_REDIRECT_BASE 0x10
void ioapic_init(u32 address);
void ioapic_write(u32 reg, u32 value);
u32 ioapic_read(u32 reg);
void ioapic_set_redirect(u32 irq, u32 apic_id, u32 vector, u16 flags);
#endif