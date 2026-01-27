#ifndef CPU_FAULTS_H
#define CPU_FAULTS_H
#include <outputs/types.h>
#include <kernel/cpu/idt.h>
typedef struct {
    const char* name;
    const char* description;
    int recoverable;
} fault_info_t;
extern fault_info_t fault_table[32];
void handle_cpu_interrupt(cpu_state_t *cpu_context);
#endif