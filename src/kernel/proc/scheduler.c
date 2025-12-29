#include "scheduler.h"
#include <string/string.h>
task_t task_pool[MAX_TASKS];
u32 next_task_id = 1;
static task_queue_t ready_queue;
static task_t* current_task = NULL;
static task_t* idle_task = NULL;
static u32 time_slice_ticks = 10;
static void idle_task_entry(void);
static void task_queue_init(task_queue_t* queue) {
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
void scheduler_init(void) {
    task_queue_init(&ready_queue);
    idle_task = task_create("idle", idle_task_entry, 0, 4096);
    if (!idle_task) {
        while (1) __asm__ volatile("hlt");
    }
    current_task = idle_task;
    current_task->state = TASK_RUNNING;
}
task_t* task_create(const char* name, void (*entry)(void), u32 priority, u64 stack_size) {
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
    task_queue_push(&ready_queue, task);
    return task;
}
void task_yield(void) {
    if (current_task) {
        schedule();
    }
}
void schedule(void) {
    task_t* next_task = NULL;
    next_task = task_queue_pop(&ready_queue);
    if (!next_task) {
        next_task = idle_task;
    }
    if (next_task == current_task) {
        current_task->ticks_remaining = time_slice_ticks;
        return;
    }
    if (current_task && current_task->state == TASK_RUNNING) {
        current_task->state = TASK_READY;
        task_queue_push(&ready_queue, current_task);
    }
    current_task = next_task;
    current_task->state = TASK_RUNNING;
    current_task->ticks_remaining = time_slice_ticks;
    if (current_task != idle_task) {
        current_task->entry_point();
    } else {
        idle_task_entry();
    }
}
void scheduler_timer_handler(void) {
    if (!current_task) return;
    if (current_task->ticks_remaining > 0) {
        current_task->ticks_remaining--;
    }
    if (current_task->ticks_remaining == 0) {
        schedule();
    }
}
task_t* scheduler_get_current_task(void) {
    return current_task;
}
void task_block(void) {
    if (current_task) {
        current_task->state = TASK_BLOCKED;
        schedule();
    }
}
void task_unblock(task_t* task) {
    if (task && task->state == TASK_BLOCKED) {
        task->state = TASK_READY;
        task_queue_push(&ready_queue, task);
    }
}
void task_terminate(task_t* task) {
    if (!task) return;
    task->state = TASK_TERMINATED;
    if (task == current_task) {
        current_task = NULL;
        schedule();
    }
}
static void idle_task_entry(void) {
    while (1) {
        __asm__ volatile("hlt");
    }
}