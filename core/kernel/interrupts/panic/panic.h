#ifndef PANIC_H
#define PANIC_H
#include <outputs/types.h>
#include <kernel/cpu/idt.h>
typedef struct {
    const char* title;
    const char* details;
    cpu_state_t* cpu_state;
} panic_context_t;
__attribute__((noreturn)) void initiate_panic(const char *message);
__attribute__((noreturn)) void initiate_fault_panic(cpu_state_t *state, const char *message);
#endif