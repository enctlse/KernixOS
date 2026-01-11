#include "per_cpu.h"
#include "smp.h"
#include <string/string.h>
#include <drivers/memory/mem.h>
#include <kernel/proc/scheduler.h>
static per_cpu_data_t per_cpu_data[MAX_CPUS];
void per_cpu_init(void) {
    memset(per_cpu_data, 0, sizeof(per_cpu_data));
    for (int i = 0; i < cpu_count; i++) {
        per_cpu_data[i].cpu_id = i;
        per_cpu_data[i].apic_id = cpu_list[i].apic_id;
        per_cpu_data[i].is_online = cpu_list[i].online;
        spinlock_init(&per_cpu_data[i].lock);
        per_cpu_data[i].current_task = NULL;
        per_cpu_data[i].idle_task = NULL;
        for (int j = 0; j < MAX_PRIORITY; j++) {
            task_queue_init(&per_cpu_data[i].ready_queues[j]);
        }
        per_cpu_data[i].task_count = 0;
    }
}
per_cpu_data_t* per_cpu_get(u32 cpu_id) {
    if (cpu_id >= cpu_count) return NULL;
    return &per_cpu_data[cpu_id];
}
per_cpu_data_t* per_cpu_current(void) {
    u32 apic_id = smp_get_current_apic_id();
    for (int i = 0; i < cpu_count; i++) {
        if (per_cpu_data[i].apic_id == apic_id) {
            return &per_cpu_data[i];
        }
    }
    return NULL;
}
void per_cpu_set_online(u32 cpu_id, int online) {
    if (cpu_id >= cpu_count) return;
    per_cpu_data[cpu_id].is_online = online;
}