#include "irq.h"
#include <kernel/cpu/idt.h>
#include <kernel/include/ports.h>
static irq_handler_t irq_handlers[16];
static volatile int irq_initialized = 0;
static void pic_remap(void)
{
    outb(PIC1_COMMAND, 0x11);
    io_wait();
    outb(PIC2_COMMAND, 0x11);
    io_wait();
    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();
    outb(PIC1_DATA, 0x04);
    io_wait();
    outb(PIC2_DATA, 0x02);
    io_wait();
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();
    outb(PIC1_DATA, 0xFF);
    io_wait();
    outb(PIC2_DATA, 0xFF);
    io_wait();
}
void irq_install(void)
{
    for (int i = 0; i < 16; i++) {
        irq_handlers[i] = NULL;
    }
    pic_remap();
    idt_set_gate(32, (u64)irq0, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(33, (u64)irq1, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(34, (u64)irq2, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(35, (u64)irq3, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(36, (u64)irq4, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(37, (u64)irq5, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(38, (u64)irq6, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(39, (u64)irq7, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(40, (u64)irq8, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(41, (u64)irq9, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(42, (u64)irq10, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(43, (u64)irq11, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(44, (u64)irq12, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(45, (u64)irq13, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(46, (u64)irq14, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    idt_set_gate(47, (u64)irq15, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_INT);
    irq_set_mask(0, 0);
    irq_set_mask(1, 0);
    irq_set_mask(12, 0);
    irq_set_mask(2, 0);
    irq_initialized = 1;
}
void irq_register_handler(u8 irq, irq_handler_t handler)
{
    if (irq < 16) {
        irq_handlers[irq] = handler;
        irq_set_mask(irq, 0);
    }
}
void irq_unregister_handler(u8 irq)
{
    if (irq < 16) {
        irq_handlers[irq] = NULL;
        irq_set_mask(irq, 1);
    }
}
void irq_set_mask(u8 irq, int enable)
{
    u16 port;
    u8 value;
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    if (enable) {
        value = inb(port) | (1 << irq);
    } else {
        value = inb(port) & ~(1 << irq);
    }
    outb(port, value);
    io_wait();
}
void irq_ack(u8 irq)
{
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}
void irq_handler(cpu_state_t* state)
{
    if (!irq_initialized) {
        return;
    }
    u8 irq = state->int_no - 32;
    if (irq < 16 && irq_handlers[irq] != NULL) {
        irq_handlers[irq](state);
    }
    irq_ack(irq);
}