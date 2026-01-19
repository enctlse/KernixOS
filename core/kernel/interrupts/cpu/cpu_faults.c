#include "cpu_faults.h"
#include "../panic/panic.h"
#include "../timer/timer.h"
#include <kernel/include/io.h>
#include <drivers/ps2/keyboard/keyboard.h>
#include <drivers/ps2/mouse/mouse.h>
fault_info_t fault_table[32] = {
    {"Division By Zero", "Division by zero error", 0},
    {"Page Fault", "Invalid memory access", 0},
    {"General Protection Fault", "Access violation", 0},
    {"Unknown Fault", "Unknown CPU fault", 0}
};
void handle_cpu_interrupt(cpu_state_t *cpu_context) {
    uint8_t fault_code = cpu_context->int_no;
    if (fault_code < 32) {
        fault_info_t *info = &fault_table[fault_code];
        if (info->recoverable) {
        }
        initiate_fault_panic(cpu_context, info->name);
    } else if (fault_code >= 32 && fault_code < 48) {
        uint8_t irq = fault_code - 32;
        switch (irq) {
            case 0:
                timer_handle_interrupt();
                break;
            case 1:
                keyboard_interrupt_handler(cpu_context);
                break;
            case 12:
                mouse_interrupt_handler(cpu_context);
                break;
            default:
                break;
        }
    } else {
        initiate_fault_panic(cpu_context, "Unknown Fault");
    }
}