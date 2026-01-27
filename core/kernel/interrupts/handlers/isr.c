#include "isr.h"
#include "../cpu/cpu_faults.h"
#include <kernel/cpu/idt.h>
#define EXCEPTION_VECTORS_COUNT 32
static isr_handler_t vector_callbacks[EXCEPTION_VECTORS_COUNT];
void exception_processor(cpu_state_t* ctx) {
    uint8_t vector_id = ctx->int_no;
    if (vector_id < EXCEPTION_VECTORS_COUNT && 
        vector_callbacks[vector_id] != NULL) {
        vector_callbacks[vector_id](ctx);
    } else {
        handle_cpu_interrupt(ctx);
    }
}
void isr_install(void) {
    for (uint8_t i = 0; i < EXCEPTION_VECTORS_COUNT; i++) {
        vector_callbacks[i] = NULL;
    }
    idt_set_gate(0, (uint64_t)exception_0, 0);
    idt_set_gate(1, (uint64_t)exception_1, 0);
    idt_set_gate(2, (uint64_t)exception_2, 0);
    idt_set_gate(3, (uint64_t)exception_3, 0);
    idt_set_gate(4, (uint64_t)exception_4, 0);
    idt_set_gate(5, (uint64_t)exception_5, 0);
    idt_set_gate(6, (uint64_t)exception_6, 0);
    idt_set_gate(7, (uint64_t)exception_7, 0);
    idt_set_gate(8, (uint64_t)exception_8, 0);
    idt_set_gate(9, (uint64_t)exception_9, 0);
    idt_set_gate(10, (uint64_t)exception_10, 0);
    idt_set_gate(11, (uint64_t)exception_11, 0);
    idt_set_gate(12, (uint64_t)exception_12, 0);
    idt_set_gate(13, (uint64_t)exception_13, 0);
    idt_set_gate(14, (uint64_t)exception_14, 0);
    idt_set_gate(15, (uint64_t)exception_15, 0);
    idt_set_gate(16, (uint64_t)exception_16, 0);
    idt_set_gate(17, (uint64_t)exception_17, 0);
    idt_set_gate(18, (uint64_t)exception_18, 0);
    idt_set_gate(19, (uint64_t)exception_19, 0);
    idt_set_gate(20, (uint64_t)exception_20, 0);
    idt_set_gate(21, (uint64_t)exception_21, 0);
    idt_set_gate(22, (uint64_t)exception_22, 0);
    idt_set_gate(23, (uint64_t)exception_23, 0);
    idt_set_gate(24, (uint64_t)exception_24, 0);
    idt_set_gate(25, (uint64_t)exception_25, 0);
    idt_set_gate(26, (uint64_t)exception_26, 0);
    idt_set_gate(27, (uint64_t)exception_27, 0);
    idt_set_gate(28, (uint64_t)exception_28, 0);
    idt_set_gate(29, (uint64_t)exception_29, 0);
    idt_set_gate(30, (uint64_t)exception_30, 0);
    idt_set_gate(31, (uint64_t)exception_31, 0);
}
void isr_register_handler(uint8_t vector, isr_handler_t callback) {
    if (vector < EXCEPTION_VECTORS_COUNT) {
        vector_callbacks[vector] = callback;
    }
}
void isr_unregister_handler(uint8_t vector) {
    if (vector < EXCEPTION_VECTORS_COUNT) {
        vector_callbacks[vector] = NULL;
    }
}