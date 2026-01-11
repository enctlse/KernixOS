#include "panic.h"
#include <kernel/graph/graphics.h>
#include <config/boot.h>
#include <kernel/graph/theme.h>
__attribute__((noreturn)) void panic(const char *message)
{
    setcontext(THEME_PANIC);
    clear(black);
    __asm__ volatile("cli");
    print("\n", theme_white());
    print("!!! --- KERNEL PANIC --- !!!", theme_red());
    print(" :(", theme_red());
    print("\n", theme_white());
    if (message) {
        print(message, theme_red());
        print("\n", theme_white());
    }
    print("\nThe system has encountered a fatal error and cannot continue.", theme_white());
    while(1) {
        __asm__ volatile("cli; hlt");
    }
}
__attribute__((noreturn)) void panic_exception(cpu_state_t *state, const char *message)
{
    setcontext(THEME_PANIC);
    clear(black);
    __asm__ volatile("cli");
    print("\n", theme_white());
    print("!!! PANIC !!!", theme_red());
    print(" :(", theme_red());
    print("\n", theme_white());
    if (message) {
        char buf[128];
        str_copy(buf, "Exception: ");
        str_append(buf, message);
        print(buf, theme_red());
        print("\n", theme_white());
    }
    char buf[128];
    str_copy(buf, "INT: ");
    str_append_uint(buf, (u32)state->int_no);
    str_append(buf, " ERR: ");
    str_append_uint(buf, (u32)state->err_code);
    print(buf, theme_white());
    print("\n", theme_white());
    str_copy(buf, "RIP: 0x");
    str_append_uint(buf, (u32)(state->rip >> 32));
    str_append_uint(buf, (u32)(state->rip & 0xFFFFFFFF));
    print(buf, theme_white());
    print("\n", theme_white());
    str_copy(buf, "RSP: 0x");
    str_append_uint(buf, (u32)(state->rsp >> 32));
    str_append_uint(buf, (u32)(state->rsp & 0xFFFFFFFF));
    print(buf, theme_white());
    print("\n\n", theme_white());
    print("The system has encountered a fatal error and cannot continue.", theme_white());
    while(1) {
        __asm__ volatile("cli; hlt");
    }
}