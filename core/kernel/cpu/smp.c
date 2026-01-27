#include "smp.h"
#include "apic.h"
#include "acpi.h"
#include "mp.h"
#include "gdt.h"
#include "idt.h"
#include <kernel/include/defs.h>
#include <string/string.h>
#include <drivers/memory/mem.h>
#include <ui/theme/colors.h>
#include <config/boot.h>
#include <kernel/processes/scheduler.h>
#include <kernel/cpu/per_cpu.h>
cpu_t cpu_list[MAX_CPUS];
int cpu_count = 0;
static u64 ap_stacks[MAX_CPUS][1024];
static void smp_add_cpu(u32 apic_id, u32 processor_id) {
    if (cpu_count >= MAX_CPUS) return;
    cpu_list[cpu_count].apic_id = apic_id;
    cpu_list[cpu_count].processor_id = processor_id;
    cpu_list[cpu_count].enabled = 1;
    cpu_list[cpu_count].online = 0;
    cpu_count++;
}
void smp_init(void) {
    SYSTEM_PRINT("[SMP] Starting smp_init()\n", gray_70);
    SYSTEM_PRINT("[SMP] About to memset cpu_list\n", gray_70);
    memset(cpu_list, 0, sizeof(cpu_list));
    cpu_count = 0;
    SYSTEM_PRINT("[SMP] memset cpu_list done\n", gray_70);
    SYSTEM_PRINT("[SMP] About to print initializing SMP support\n", gray_70);
    SYSTEM_PRINT("[SMP] Initializing SMP support\n", gray_70);
    SYSTEM_PRINT("[SMP] Print done, calling apic_init()\n", gray_70);
    SYSTEM_PRINT("[SMP] Calling apic_init()\n", gray_70);
    apic_init();
    SYSTEM_PRINT("[SMP] apic_init() completed\n", gray_70);
    if (mp_request.response && mp_request.response->cpu_count > 0) {
        SYSTEM_PRINT("[SMP] Using Limine SMP request for CPU detection\n", gray_70);
        for (u64 i = 0; i < mp_request.response->cpu_count; i++) {
            struct limine_mp_info* cpu = mp_request.response->cpus[i];
            SYSTEM_PRINT("[SMP] CPU ", gray_70);
            SYSTEM_PRINT_INT(i, cyan);
            SYSTEM_PRINT(" APIC ID ", gray_70);
            SYSTEM_PRINT_INT(cpu->lapic_id, cyan);
            SYSTEM_PRINT(" Processor ID ", gray_70);
            SYSTEM_PRINT_INT(cpu->processor_id, cyan);
            SYSTEM_PRINT("\n", gray_70);
            smp_add_cpu(cpu->lapic_id, cpu->processor_id);
        }
    } else {
        SYSTEM_PRINT("[SMP] Limine MP request not available, assuming single CPU\n", red);
        smp_add_cpu(apic_get_id(), 0);
    }
    if (cpu_count > 0) {
        cpu_list[0].online = 1;
    }
    SYSTEM_PRINT("[SMP] Detected ", gray_70);
    SYSTEM_PRINT_INT(cpu_count, cyan);
    SYSTEM_PRINT(" CPUs\n", gray_70);
}
void smp_start_aps(void) {
    if (!mp_request.response) return;
    __asm__ volatile("cli");
    for (u64 i = 1; i < mp_request.response->cpu_count; i++) {
        struct limine_mp_info* cpu = mp_request.response->cpus[i];
        cpu->goto_address = ap_entry;
    }
    __asm__ volatile("sti");
}
int smp_get_cpu_count(void) {
    return cpu_count;
}
int smp_get_online_cpus(void) {
    int online = 0;
    for (int i = 0; i < cpu_count; i++) {
        if (cpu_list[i].online) online++;
    }
    return online;
}
void smp_send_ipi(u32 apic_id, u32 vector) {
    apic_send_ipi(apic_id, vector);
}
u32 smp_get_current_apic_id(void) {
    return apic_get_id();
}
void ap_entry(struct limine_mp_info* info) {
    u32 apic_id = info->lapic_id;
    int cpu_index = -1;
    for (int i = 0; i < cpu_count; i++) {
        if (cpu_list[i].apic_id == apic_id) {
            cpu_index = i;
            break;
        }
    }
    if (cpu_index == -1) {
        while (1) __asm__ volatile("hlt");
    }
    u64 *stack = ap_stacks[cpu_index] + 1024;
    __asm__ volatile("mov %0, %%rsp" : : "r"(stack) : "memory");
    per_cpu_set_online(cpu_index, 1);
    __asm__ volatile("sti");
    ap_main();
}
void ap_main(void) {
    initialize_ap_scheduler();
    __asm__ volatile("sti");
    per_cpu_data_t* pcpu = per_cpu_current();
    if (pcpu && pcpu->current_task) {
        pcpu->current_task->start_function();
    }
    while (1) {
        __asm__ volatile("hlt");
    }
}