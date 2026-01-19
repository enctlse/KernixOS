#include "panic.h"
#include <kernel/display/visual.h>
#include <config/boot.h>

static void display_panic_header() {
    setcontext(THEME_PANIC);
    clear(black);
    __asm__ volatile("cli");
    print("\n", theme_white);
    print("*** SYSTEM FAILURE DETECTED ***", theme_red);
    print("\n", theme_white);
}

static void display_cpu_state(cpu_state_t *state) {
    if (!state) return;
    char buffer[256];
    str_copy(buffer, "Vector: ");
    str_append_uint(buffer, (u32)state->int_no);
    str_append(buffer, " | Error: ");
    str_append_uint(buffer, (u32)state->err_code);
    print(buffer, theme_white);
    print("\n", theme_white);

    str_copy(buffer, "Instruction Pointer: 0x");
    str_append_hex(buffer, state->rip);
    print(buffer, theme_white);
    print("\n", theme_white);

    str_copy(buffer, "Stack Pointer: 0x");
    str_append_hex(buffer, state->rsp);
    print(buffer, theme_white);
    print("\n", theme_white);
}

static void __attribute__((noreturn)) halt_system() {
    print("\nSystem halted. Manual restart required.\n", theme_white);
    while(1) {
        __asm__ volatile("cli; hlt");
    }
    __builtin_unreachable();
}

__attribute__((noreturn)) void initiate_panic(const char *message) {
    display_panic_header();
    if (message) {
        print("Reason: ", theme_red);
        print(message, theme_red);
        print("\n", theme_white);
    }
    print("Critical system error occurred.\n", theme_white);
    halt_system();
    __builtin_unreachable();
}

__attribute__((noreturn)) void initiate_fault_panic(cpu_state_t *state, const char *message) {
    display_panic_header();
    if (message) {
        print("Exception Type: ", theme_red);
        print(message, theme_red);
        print("\n", theme_white);
    }
    display_cpu_state(state);
    print("Processor exception triggered system shutdown.\n", theme_white);
    halt_system();
    __builtin_unreachable();
}