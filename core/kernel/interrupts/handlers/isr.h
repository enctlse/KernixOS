#ifndef INTERRUPT_SERVICE_H
#define INTERRUPT_SERVICE_H

#include <outputs/types.h>
#include <kernel/cpu/idt.h>

extern void exception_31(void);
extern void exception_30(void);
extern void exception_29(void);
extern void exception_28(void);
extern void exception_27(void);
extern void exception_26(void);
extern void exception_25(void);
extern void exception_24(void);
extern void exception_23(void);
extern void exception_22(void);
extern void exception_21(void);
extern void exception_20(void);
extern void exception_19(void);
extern void exception_18(void);
extern void exception_17(void);
extern void exception_16(void);
extern void exception_15(void);
extern void exception_14(void);
extern void exception_13(void);
extern void exception_12(void);
extern void exception_11(void);
extern void exception_10(void);
extern void exception_9(void);
extern void exception_8(void);
extern void exception_7(void);
extern void exception_6(void);
extern void exception_5(void);
extern void exception_4(void);
extern void exception_3(void);
extern void exception_2(void);
extern void exception_1(void);
extern void exception_0(void);

typedef void (*isr_handler_t)(cpu_state_t* state);

void isr_unregister_handler(u8 num);
void isr_register_handler(u8 num, isr_handler_t handler);
void isr_install(void);

#endif