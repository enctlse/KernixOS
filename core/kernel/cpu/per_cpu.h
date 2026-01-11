#ifndef PER_CPU_H
#define PER_CPU_H
#include <outputs/types.h>
#include <kernel/cpu/spinlock.h>
#include <kernel/proc/scheduler.h>
typedef struct {
    u32 cpu_id;
    u32 apic_id;
    u32 is_online;
    u64 idle_time;
    u64 user_time;
    u64 kernel_time;
    u32 interrupt_nesting;
    void* current_process;
    void* current_thread;
    task_t* current_task;
    task_t* idle_task;
    task_queue_t ready_queues[MAX_PRIORITY];
    spinlock_t lock;
    u32 task_count;
} per_cpu_data_t;
void per_cpu_init(void);
per_cpu_data_t* per_cpu_get(u32 cpu_id);
per_cpu_data_t* per_cpu_current(void);
void per_cpu_set_online(u32 cpu_id, int online);
#endif