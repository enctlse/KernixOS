#ifndef ULIME_H
#define ULIME_H
#include <outputs/types.h>
#include "../mem.h"
#include "../kernel_memory/kernel_memory.h"
#include "../graphics_manager/graphics_manager.h"
#include "../phys/physmem.h"
typedef struct user_space_proc {
    u64 pid;
    u8  name[64];
    u64 priority;
    u64 state;
    u64 *ptr_pagetable;
    u64 entry_point;
    u64 heap_base;
    u64 heap_size;
    u64 stack_base;
    u64 stack_size;
    u64 phys_heap;
    u64 phys_stack;
    struct user_space *user_space;
    struct user_space_proc *next;
    struct user_space_proc *prev;
} user_space_proc_t;
#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_GETPID  3
typedef u64 (*syscall_handler_t)(user_space_proc_t *proc, u64 arg1, u64 arg2, u64 arg3);
typedef struct user_space_thread {
    u64 tid;
    user_space_proc_t *ptr_proc;
    u64 *ptr_stack;
    u64 *ptr_instruction;
    u64 state;
} user_space_thread_t;
typedef struct user_space {
    user_space_proc_t   *ptr_proc_list;
    user_space_proc_t   *ptr_proc_curr;
    user_space_thread_t *ptr_thread;
    u64 pid_next;
    u64 tid_next;
    kernel_memory_t *kernel_memory;
    graphics_manager_t *graphics_manager;
    limine_hhdm_response_t *hpr;
    syscall_handler_t syscalls[256];
    u64 internal_pool_base;
    u64 internal_pool_size;
    u64 internal_pool_used;
    u64 user_space_base;
    u64 user_space_size;
    u64 user_space_used;
} user_space_t;
#define PROC_CREATED    1
#define PROC_READY      2
#define PROC_RUNNING    3
#define PROC_BLOCKED    4
#define PROC_ZOMBIE     5
user_space_t *user_space_init(limine_hhdm_response_t *hpr, kernel_memory_t *kernel_memory, graphics_manager_t *graphics_manager, u64 uphys_start);
void user_space_init_syscalls(user_space_t *user_space);
user_space_proc_t *user_space_proc_create(user_space_t *user_space, u8 *name, u64 entry_point);
int user_space_proc_kill(user_space_t *user_space, u64 pid);
int user_space_proc_mmap(user_space_t *user_space, user_space_proc_t *proc);
void user_space_proc_test_mem(user_space_proc_t *proc);
void user_space_schedule(user_space_t *user_space);
void user_space_proc_list(user_space_t *user_space);
int user_space_load_program(user_space_proc_t *proc, u8 *code, u64 code_size);
#endif