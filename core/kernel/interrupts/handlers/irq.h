#ifndef INTERRUPT_HANDLER_H
#define INTERRUPT_HANDLER_H
#include <outputs/types.h>
#include <kernel/cpu/idt.h>
#define TIMER_VECTOR       32
#define KEYBOARD_VECTOR    33
#define CASCADE_VECTOR     34
#define SERIAL2_VECTOR     35
#define SERIAL1_VECTOR     36
#define PARALLEL2_VECTOR   37
#define FLOPPY_VECTOR      38
#define PARALLEL1_VECTOR   39
#define RTC_VECTOR         40
#define UNUSED1_VECTOR     41
#define UNUSED2_VECTOR     42
#define UNUSED3_VECTOR     43
#define MOUSE_VECTOR       44
#define FPU_VECTOR         45
#define PRIMARY_ATA_VECTOR 46
#define SECONDARY_ATA_VECTOR 47
#define PRIMARY_PIC_CMD  0x20
#define PRIMARY_PIC_DATA 0x21
#define SECONDARY_PIC_CMD 0xA0
#define SECONDARY_PIC_DATA 0xA1
#define EOI_SIGNAL       0x20
typedef void (*interrupt_callback)(cpu_state_t* state);
extern void hardware_interrupt_15(void);
extern void hardware_interrupt_14(void);
extern void hardware_interrupt_13(void);
extern void hardware_interrupt_12(void);
extern void hardware_interrupt_11(void);
extern void hardware_interrupt_10(void);
extern void hardware_interrupt_9(void);
extern void hardware_interrupt_8(void);
extern void hardware_interrupt_7(void);
extern void hardware_interrupt_6(void);
extern void hardware_interrupt_5(void);
extern void hardware_interrupt_4(void);
extern void hardware_interrupt_3(void);
extern void hardware_interrupt_2(void);
extern void hardware_interrupt_1(void);
extern void hardware_interrupt_0(void);
void irq_ack(u8 num);
void irq_set_mask(u8 num, int enable);
void irq_register_handler(u8 num, interrupt_callback handler);
void irq_unregister_handler(u8 num);
void irq_install(void);
#endif