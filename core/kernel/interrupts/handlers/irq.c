#include <kernel/include/io.h>
#include <kernel/cpu/idt.h>
#include "irq.h"
static interrupt_callback handlers_array[16];
static volatile int system_ready = 0;
void interrupt_processor(cpu_state_t* context) {
    if (!system_ready) {
        return;
    }
    u8 interrupt_num = context->int_no - 32;
    if (interrupt_num < 16 && handlers_array[interrupt_num] != NULL) {
        handlers_array[interrupt_num](context);
    }
    if (interrupt_num >= 8) {
        outb(SECONDARY_PIC_CMD, EOI_SIGNAL);
    }
    outb(PRIMARY_PIC_CMD, EOI_SIGNAL);
}
static void reprogram_pic(void) {
    outb(PRIMARY_PIC_CMD, 0x11);
    io_wait();
    outb(SECONDARY_PIC_CMD, 0x11);
    io_wait();
    outb(PRIMARY_PIC_DATA, 0x20);
    io_wait();
    outb(SECONDARY_PIC_DATA, 0x28);
    io_wait();
    outb(PRIMARY_PIC_DATA, 0x04);
    io_wait();
    outb(SECONDARY_PIC_DATA, 0x02);
    io_wait();
    outb(PRIMARY_PIC_DATA, 0x01);
    io_wait();
    outb(SECONDARY_PIC_DATA, 0x01);
    io_wait();
    outb(PRIMARY_PIC_DATA, 0xFF);
    io_wait();
    outb(SECONDARY_PIC_DATA, 0xFF);
    io_wait();
}
void irq_ack(u8 num) {
    if (num >= 8) {
        outb(SECONDARY_PIC_CMD, EOI_SIGNAL);
    }
    outb(PRIMARY_PIC_CMD, EOI_SIGNAL);
}
void irq_set_mask(u8 num, int enable) {
    u16 port_addr;
    u8 current_mask;
    if (num < 8) {
        port_addr = PRIMARY_PIC_DATA;
    } else {
        port_addr = SECONDARY_PIC_DATA;
        num -= 8;
    }
    if (enable) {
        current_mask = inb(port_addr) | (1 << num);
    } else {
        current_mask = inb(port_addr) & ~(1 << num);
    }
    outb(port_addr, current_mask);
    io_wait();
}
void irq_register_handler(u8 num, interrupt_callback routine) {
    if (num < 16) {
        handlers_array[num] = routine;
        irq_set_mask(num, 0);
    }
}
void irq_unregister_handler(u8 num) {
    if (num < 16) {
        handlers_array[num] = NULL;
        irq_set_mask(num, 1);
    }
}
void irq_install(void) {
    for (int idx = 0; idx < 16; idx++) {
        handlers_array[idx] = NULL;
    }
    reprogram_pic();
    idt_set_gate(32, (u64)hardware_interrupt_0, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(33, (u64)hardware_interrupt_1, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(34, (u64)hardware_interrupt_2, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(35, (u64)hardware_interrupt_3, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(36, (u64)hardware_interrupt_4, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(37, (u64)hardware_interrupt_5, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(38, (u64)hardware_interrupt_6, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(39, (u64)hardware_interrupt_7, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(40, (u64)hardware_interrupt_8, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(41, (u64)hardware_interrupt_9, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(42, (u64)hardware_interrupt_10, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(43, (u64)hardware_interrupt_11, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(44, (u64)hardware_interrupt_12, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(45, (u64)hardware_interrupt_13, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(46, (u64)hardware_interrupt_14, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(47, (u64)hardware_interrupt_15, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    irq_set_mask(0, 0);
    irq_set_mask(1, 0);
    irq_set_mask(12, 0);
    irq_set_mask(2, 0);
    system_ready = 1;
}