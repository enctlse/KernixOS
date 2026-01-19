#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H
#include <outputs/types.h>
#define MAX_TASK_COUNT 256
#define MAX_PRIORITY_LEVELS 32
typedef enum {
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_TERMINATED
} task_status_t;
typedef struct task {
    u32 identifier;
    task_status_t status;
    u32 priority_level;
    u64* stack_pointer;
    u64* stack_start;
    u64 stack_capacity;
    void (*start_function)(void);
    u64 execution_context[20];
    char task_name[32];
    u64 remaining_ticks;
    u32 assigned_cpu;
} task_t;
typedef struct {
    task_t* task_list[MAX_TASK_COUNT];
    u32 item_count;
    u32 start_index;
    u32 end_index;
} task_queue_t;
extern task_t task_pool[MAX_TASK_COUNT];
extern u32 next_task_id;
void initialize_scheduler(void);
void initialize_ap_scheduler(void);
task_t* create_new_task(const char* name, void (*entry)(void), u32 priority, u64 stack_size);
void yield_current_task(void);
void perform_scheduling(void);
void handle_scheduler_timer(void);
task_t* get_current_task(void);
void block_current_task(void);
void unblock_task(task_t* task);
void terminate_task(task_t* task);
void initialize_task_queue(task_queue_t* queue);
extern void perform_task_switch(u64* old_context, u64* new_context);
#endif