#include "scheduler.h"
#include <string/string.h>
#include <kernel/cpu/per_cpu.h>
#include <kernel/cpu/smp.h>
#include <limits.h>
task_t task_pool[MAX_TASKS];
u32 next_task_id = 1;
static u32 time_slice_ticks = 10;
static void idle_task_entry(void);
void task_queue_init(task_queue_t* queue) {
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        queue->tasks[i] = NULL;
    }
}
static void task_queue_push(task_queue_t* queue, task_t* task) {
    if (queue->count >= MAX_TASKS) return;
    queue->tasks[queue->tail] = task;
    queue->tail = (queue->tail + 1) % MAX_TASKS;
    queue->count++;
}
static task_t* task_queue_pop(task_queue_t* queue) {
    if (queue->count == 0) return NULL;
    task_t* task = queue->tasks[queue->head];
    queue->tasks[queue->head] = NULL;
    queue->head = (queue->head + 1) % MAX_TASKS;
    queue->count--;
    return task;
}
static task_t* task_queue_pop_priority(task_queue_t queues[MAX_PRIORITY]) {
    for (int p = 0; p < MAX_PRIORITY; p++) {
        if (queues[p].count > 0) {
            return task_queue_pop(&queues[p]);
        }
    }
    return NULL;
}
void scheduler_init(void) {
    per_cpu_data_t* pcpu = per_cpu_current();
    if (!pcpu) {
        while (1) __asm__ volatile("hlt");
    }
    for (int j = 0; j < MAX_PRIORITY; j++) {
        task_queue_init(&pcpu->ready_queues[j]);
    }
    pcpu->idle_task = task_create("idle", idle_task_entry, 0, 4096);
    if (!pcpu->idle_task) {
        while (1) __asm__ volatile("hlt");
    }
    pcpu->current_task = pcpu->idle_task;
    pcpu->idle_task->state = TASK_RUNNING;
}
void scheduler_ap_init(void) {
    per_cpu_data_t* pcpu = per_cpu_current();
    if (!pcpu) {
        while (1) __asm__ volatile("hlt");
    }
    for (int j = 0; j < MAX_PRIORITY; j++) {
        task_queue_init(&pcpu->ready_queues[j]);
    }
    pcpu->idle_task = task_create("idle_ap", idle_task_entry, 0, 4096);
    if (!pcpu->idle_task) {
        while (1) __asm__ volatile("hlt");
    }
    pcpu->current_task = pcpu->idle_task;
    pcpu->idle_task->state = TASK_RUNNING;
}
task_t* task_create(const char* name, void (*entry)(void), u32 priority, u64 stack_size) {
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
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_pool[i].state == TASK_TERMINATED || task_pool[i].id == 0) {
            task = &task_pool[i];
            break;
        }
    }
    if (!task) return NULL;
    static u64 task_stacks[MAX_TASKS][1024];
    static int stack_index = 0;
    if (stack_index >= MAX_TASKS) return NULL;
    task->id = next_task_id++;
    task->state = TASK_READY;
    task->priority = priority;
    task->entry_point = entry;
    task->stack_size = stack_size;
    task->stack_base = task_stacks[stack_index];
    task->stack_top = task->stack_base + (stack_size / sizeof(u64)) - 1;
    task->ticks_remaining = time_slice_ticks;
    task->cpu_id = target_pcpu->cpu_id;
    str_copy(task->name, name);
    *(task->stack_top--) = (u64)entry;
    *(task->stack_top--) = 0;
    *(task->stack_top--) = 0;
    *(task->stack_top--) = 0;
    *(task->stack_top--) = 0;
    *(task->stack_top--) = 0;
    *(task->stack_top--) = 0;
    *(task->stack_top--) = 0x202;
    stack_index++;
    spinlock_acquire(&target_pcpu->lock);
    task_queue_push(&target_pcpu->ready_queues[priority], task);
    target_pcpu->task_count++;
    spinlock_release(&target_pcpu->lock);
    return task;
}
void task_yield(void) {
    per_cpu_data_t* pcpu = per_cpu_current();
    if (pcpu && pcpu->current_task) {
        schedule();
    }
}
void schedule(void) {
    per_cpu_data_t* pcpu = per_cpu_current();
    if (!pcpu) return;
    spinlock_acquire(&pcpu->lock);
    task_t* next_task = task_queue_pop_priority(pcpu->ready_queues);
    spinlock_release(&pcpu->lock);
    if (!next_task) {
        next_task = pcpu->idle_task;
    }
    if (next_task == pcpu->current_task) {
        pcpu->current_task->ticks_remaining = time_slice_ticks;
        return;
    }
    task_t* old_task = pcpu->current_task;
    if (old_task && old_task->state == TASK_RUNNING) {
        old_task->state = TASK_READY;
        spinlock_acquire(&pcpu->lock);
        task_queue_push(&pcpu->ready_queues[old_task->priority], old_task);
        spinlock_release(&pcpu->lock);
    }
    pcpu->current_task = next_task;
    next_task->state = TASK_RUNNING;
    next_task->ticks_remaining = time_slice_ticks;
    if (next_task != pcpu->idle_task) {
        scheduler_context_switch(old_task ? old_task->context : NULL, next_task->context);
    } else {
        idle_task_entry();
    }
}
void scheduler_timer_handler(void) {
    per_cpu_data_t* pcpu = per_cpu_current();
    if (!pcpu || !pcpu->current_task) return;
    if (pcpu->current_task->ticks_remaining > 0) {
        pcpu->current_task->ticks_remaining--;
    }
    if (pcpu->current_task->ticks_remaining == 0) {
        schedule();
    }
}
task_t* scheduler_get_current_task(void) {
    per_cpu_data_t* pcpu = per_cpu_current();
    return pcpu ? pcpu->current_task : NULL;
}
void task_block(void) {
    per_cpu_data_t* pcpu = per_cpu_current();
    if (pcpu && pcpu->current_task) {
        pcpu->current_task->state = TASK_BLOCKED;
        schedule();
    }
}
void task_unblock(task_t* task) {
    if (!task || task->state != TASK_BLOCKED) return;
    per_cpu_data_t* target_pcpu = per_cpu_get(task->cpu_id);
    if (!target_pcpu) return;
    spinlock_acquire(&target_pcpu->lock);
    task->state = TASK_READY;
    task_queue_push(&target_pcpu->ready_queues[task->priority], task);
    spinlock_release(&target_pcpu->lock);
}
void task_terminate(task_t* task) {
    if (!task) return;
    task->state = TASK_TERMINATED;
    per_cpu_data_t* target_pcpu = per_cpu_get(task->cpu_id);
    if (target_pcpu) target_pcpu->task_count--;
    per_cpu_data_t* pcpu = per_cpu_current();
    if (pcpu && task == pcpu->current_task) {
        pcpu->current_task = NULL;
        schedule();
    }
}
static void idle_task_entry(void) {
    while (1) {
        __asm__ volatile("hlt");
    }
}