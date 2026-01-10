#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <types.h>
#define MAX_TASKS 256
#define MAX_PRIORITY 32
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_TERMINATED
} task_state_t;
typedef struct task {
    u32 id;
    task_state_t state;
    u32 priority;
    u64* stack_top;
    u64* stack_base;
    u64 stack_size;
    void (*entry_point)(void);
    u64 context[20];
    char name[32];
    u64 ticks_remaining;
    u32 cpu_id;
} task_t;
typedef struct {
    task_t* tasks[MAX_TASKS];
    u32 count;
    u32 head;
    u32 tail;
} task_queue_t;
extern task_t task_pool[MAX_TASKS];
extern u32 next_task_id;
void scheduler_init(void);
void scheduler_ap_init(void);
task_t* task_create(const char* name, void (*entry)(void), u32 priority, u64 stack_size);
void task_yield(void);
void schedule(void);
void scheduler_timer_handler(void);
task_t* scheduler_get_current_task(void);
void task_block(void);
void task_unblock(task_t* task);
void task_terminate(task_t* task);
void task_queue_init(task_queue_t* queue);
extern void scheduler_context_switch(u64* old_context, u64* new_context);
#endif