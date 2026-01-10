#ifndef PANIC_H
#define PANIC_H
#include <types.h>
#include <kernel/cpu/idt.h>
__attribute__((noreturn)) void panic(const char *message);
__attribute__((noreturn)) void panic_exception(cpu_state_t *state, const char *message);
#endif