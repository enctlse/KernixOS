#include "smp.h"
#include "apic.h"
#include "acpi.h"
#include "mp.h"
#include "gdt.h"
#include "idt.h"
#include <kernel/include/reqs.h>
#include <string/string.h>
#include <memory/main.h>
#include <theme/stdclrs.h>
#include <kernel/graph/theme.h>
#include <theme/tmx.h>
#include <kernel/proc/scheduler.h>
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
    BOOTUP_PRINT("[SMP] Starting smp_init()\n", GFX_GRAY_70);
    BOOTUP_PRINT("[SMP] About to memset cpu_list\n", GFX_GRAY_70);
    memset(cpu_list, 0, sizeof(cpu_list));
    cpu_count = 0;
    BOOTUP_PRINT("[SMP] memset cpu_list done\n", GFX_GRAY_70);
    BOOTUP_PRINT("[SMP] About to print initializing SMP support\n", GFX_GRAY_70);
    BOOTUP_PRINT("[SMP] Initializing SMP support\n", GFX_GRAY_70);
    BOOTUP_PRINT("[SMP] Print done, calling apic_init()\n", GFX_GRAY_70);
    BOOTUP_PRINT("[SMP] Calling apic_init()\n", GFX_GRAY_70);
    apic_init();
    BOOTUP_PRINT("[SMP] apic_init() completed\n", GFX_GRAY_70);
    if (mp_request.response && mp_request.response->cpu_count > 0) {
        BOOTUP_PRINT("[SMP] Using Limine MP request for CPU detection\n", GFX_GRAY_70);
        for (u64 i = 0; i < mp_request.response->cpu_count; i++) {
            struct limine_mp_info* cpu = mp_request.response->cpus[i];
            BOOTUP_PRINT("[SMP] CPU ", GFX_GRAY_70);
            BOOTUP_PRINT_INT(i, GFX_CYAN);
            BOOTUP_PRINT(" APIC ID ", GFX_GRAY_70);
            BOOTUP_PRINT_INT(cpu->lapic_id, GFX_CYAN);
            BOOTUP_PRINT(" Processor ID ", GFX_GRAY_70);
            BOOTUP_PRINT_INT(cpu->processor_id, GFX_CYAN);
            BOOTUP_PRINT("\n", GFX_GRAY_70);
            smp_add_cpu(cpu->lapic_id, cpu->processor_id);
        }
    } else {
        BOOTUP_PRINT("[SMP] Limine MP request not available, assuming single CPU\n", GFX_RED);
        smp_add_cpu(apic_get_id(), 0);
    }
    if (cpu_count > 0) {
        cpu_list[0].online = 1;
    }
    BOOTUP_PRINT("[SMP] Detected ", GFX_GRAY_70);
    BOOTUP_PRINT_INT(cpu_count, GFX_CYAN);
    BOOTUP_PRINT(" CPUs\n", GFX_GRAY_70);
}
void smp_start_aps(void) {
    if (!mp_request.response) return;
    __asm__ volatile("cli");
    for (u64 i = 1; i < mp_request.response->cpu_count; i++) {
        struct limine_mp_info* cpu = mp_request.response->cpus[i];
        limine_goto_address original = cpu->goto_address;
        cpu->goto_address = ap_entry;
        original(cpu);
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
    scheduler_ap_init();
    __asm__ volatile("sti");
    per_cpu_data_t* pcpu = per_cpu_current();
    if (pcpu && pcpu->current_task) {
        pcpu->current_task->entry_point();
    }
    while (1) {
        __asm__ volatile("hlt");
    }
}