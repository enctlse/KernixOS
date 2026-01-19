#include "scheduler.h"
#include <string/string.h>
#include <kernel/cpu/per_cpu.h>
#include <kernel/cpu/smp.h>
#include <limits.h>
task_t task_pool[MAX_TASK_COUNT];
u32 next_task_id = 1;
static u32 time_slice_ticks = 10;
static void idle_task_entry(void);
void initialize_task_queue(task_queue_t* queue) {
    queue->item_count = 0;
    queue->start_index = 0;
    queue->end_index = 0;
    for (int i = 0; i < MAX_TASK_COUNT; i++) {
        queue->task_list[i] = NULL;
    }
}
static void task_queue_push(task_queue_t* queue, task_t* task) {
    if (queue->item_count >= MAX_TASK_COUNT) return;
    queue->task_list[queue->end_index] = task;
    queue->end_index = (queue->end_index + 1) % MAX_TASK_COUNT;
    queue->item_count++;
}
static task_t* task_queue_pop(task_queue_t* queue) {
    if (queue->item_count == 0) return NULL;
    task_t* task = queue->task_list[queue->start_index];
    queue->task_list[queue->start_index] = NULL;
    queue->start_index = (queue->start_index + 1) % MAX_TASK_COUNT;
    queue->item_count--;
    return task;
}
static task_t* task_queue_pop_priority(task_queue_t queues[MAX_PRIORITY_LEVELS]) {
    for (int p = 0; p < MAX_PRIORITY_LEVELS; p++) {
        if (queues[p].item_count > 0) {
            return task_queue_pop(&queues[p]);
        }
    }
    return NULL;
}
void initialize_scheduler(void) {
    per_cpu_data_t* pcpu = per_cpu_current();
    if (!pcpu) {
        while (1) __asm__ volatile("hlt");
    }
    for (int j = 0; j < MAX_PRIORITY_LEVELS; j++) {
        initialize_task_queue(&pcpu->ready_queues[j]);
    }
    pcpu->idle_task = create_new_task("idle", idle_task_entry, 0, 4096);
    if (!pcpu->idle_task) {
        while (1) __asm__ volatile("hlt");
    }
    pcpu->current_task = pcpu->idle_task;
    pcpu->idle_task->status = TASK_STATE_RUNNING;
}
void initialize_ap_scheduler(void) {
    per_cpu_data_t* pcpu = per_cpu_current();
    if (!pcpu) {
        while (1) __asm__ volatile("hlt");
    }
    for (int j = 0; j < MAX_PRIORITY_LEVELS; j++) {
        initialize_task_queue(&pcpu->ready_queues[j]);
    }
    pcpu->idle_task = create_new_task("idle_ap", idle_task_entry, 0, 4096);
    if (!pcpu->idle_task) {
        while (1) __asm__ volatile("hlt");
    }
    pcpu->current_task = pcpu->idle_task;
    pcpu->idle_task->status = TASK_STATE_RUNNING;
}
task_t* create_new_task(const char* name, void (*entry)(void), u32 priority, u64 stack_size) {
    per_cpu_data_t* target_pcpu = NULL;
    u32 min_count = UINT32_MAX;
    for (int i = 0; i < cpu_count; i++) {
        per_cpu_data_t* pcpu = per_cpu_get(i);
        if (pcpu && pcpu->is_online && pcpu->task_count < min_count) {
            min_count = pcpu->task_count;
            target_pcpu = pcpu;
        }
    }
    if (!target_pcpu) {
        per_cpu_data_t* pcpu = per_cpu_current();
        target_pcpu = pcpu;
    }
    task_t* task = NULL;
    for (int i = 0; i < MAX_TASK_COUNT; i++) {
        if (task_pool[i].status == TASK_STATE_TERMINATED || task_pool[i].identifier == 0) {
            task = &task_pool[i];
            break;
        }
    }
    if (!task) return NULL;
    static u64 task_stacks[MAX_TASK_COUNT][1024];
    static int stack_index = 0;
    if (stack_index >= MAX_TASK_COUNT) return NULL;
    task->identifier = next_task_id++;
    task->status = TASK_STATE_READY;
    task->priority_level = priority;
    task->start_function = entry;
    task->stack_capacity = stack_size;
    task->stack_start = task_stacks[stack_index];
    task->stack_pointer = task->stack_start + (stack_size / sizeof(u64)) - 1;
    task->remaining_ticks = time_slice_ticks;
    task->assigned_cpu = target_pcpu->cpu_id;
    str_copy(task->task_name, name);
    *(task->stack_pointer--) = (u64)entry;
    *(task->stack_pointer--) = 0;
    *(task->stack_pointer--) = 0;
    *(task->stack_pointer--) = 0;
    *(task->stack_pointer--) = 0;
    *(task->stack_pointer--) = 0;
    *(task->stack_pointer--) = 0;
    *(task->stack_pointer--) = 0x202;
    stack_index++;
    spinlock_acquire(&target_pcpu->lock);
    task_queue_push(&target_pcpu->ready_queues[priority], task);
    target_pcpu->task_count++;
    spinlock_release(&target_pcpu->lock);
    return task;
}
void yield_current_task(void) {
    per_cpu_data_t* pcpu = per_cpu_current();
    if (pcpu && pcpu->current_task) {
        perform_scheduling();
    }
}
void perform_scheduling(void) {
    per_cpu_data_t* pcpu = per_cpu_current();
    if (!pcpu) return;
    spinlock_acquire(&pcpu->lock);
    task_t* next_task = task_queue_pop_priority(pcpu->ready_queues);
    spinlock_release(&pcpu->lock);
    if (!next_task) {
        next_task = pcpu->idle_task;
    }
    if (next_task == pcpu->current_task) {
        pcpu->current_task->remaining_ticks = time_slice_ticks;
        return;
    }
    task_t* old_task = pcpu->current_task;
    if (old_task && old_task->status == TASK_STATE_RUNNING) {
        old_task->status = TASK_STATE_READY;
        spinlock_acquire(&pcpu->lock);
        task_queue_push(&pcpu->ready_queues[old_task->priority_level], old_task);
        spinlock_release(&pcpu->lock);
    }
    pcpu->current_task = next_task;
    next_task->status = TASK_STATE_RUNNING;
    next_task->remaining_ticks = time_slice_ticks;
    if (next_task != pcpu->idle_task) {
        perform_task_switch(old_task ? old_task->execution_context : NULL, next_task->execution_context);
    } else {
        idle_task_entry();
    }
}
void handle_scheduler_timer(void) {
    per_cpu_data_t* pcpu = per_cpu_current();
    if (!pcpu || !pcpu->current_task) return;
    if (pcpu->current_task->remaining_ticks > 0) {
        pcpu->current_task->remaining_ticks--;
    }
    if (pcpu->current_task->remaining_ticks == 0) {
        perform_scheduling();
    }
}
task_t* get_current_task(void) {
    per_cpu_data_t* pcpu = per_cpu_current();
    return pcpu ? pcpu->current_task : NULL;
}
void block_current_task(void) {
    per_cpu_data_t* pcpu = per_cpu_current();
    if (pcpu && pcpu->current_task) {
        pcpu->current_task->status = TASK_STATE_BLOCKED;
        perform_scheduling();
    }
}
void unblock_task(task_t* task) {
    if (!task || task->status != TASK_STATE_BLOCKED) return;
    per_cpu_data_t* target_pcpu = per_cpu_get(task->assigned_cpu);
    if (!target_pcpu) return;
    spinlock_acquire(&target_pcpu->lock);
    task->status = TASK_STATE_READY;
    task_queue_push(&target_pcpu->ready_queues[task->priority_level], task);
    spinlock_release(&target_pcpu->lock);
}
void terminate_task(task_t* task) {
    if (!task) return;
    task->status = TASK_STATE_TERMINATED;
    per_cpu_data_t* target_pcpu = per_cpu_get(task->assigned_cpu);
    if (target_pcpu) target_pcpu->task_count--;
    per_cpu_data_t* pcpu = per_cpu_current();
    if (pcpu && task == pcpu->current_task) {
        pcpu->current_task = NULL;
        perform_scheduling();
    }
}
static void idle_task_entry(void) {
    while (1) {
        __asm__ volatile("hlt");
    }
}