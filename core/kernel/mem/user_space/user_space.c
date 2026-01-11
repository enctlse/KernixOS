#include "user_space.h"
#include <kernel/communication/serial.h>
#include <kernel/exceptions/panic.h>
#include <drivers/memory/mem.h>
#include <string/string.h>
#include <kernel/mem/paging/paging.h>
#include <kernel/graph/theme.h>
#include <config/boot.h>
user_space_t *user_space_init(limine_hhdm_response_t *hpr, kernel_memory_t *kernel_memory, graphics_manager_t *graphics_manager, u64 uphys_start) {
    if (!kernel_memory || !graphics_manager) return NULL;
    user_space_t *user_space = (user_space_t *)(u64 *)ULIME_START;
    memset(user_space, 0, ULIME_META_SIZE);
    user_space->kernel_memory = kernel_memory;
    user_space->graphics_manager = graphics_manager;
    user_space->ptr_proc_list = NULL;
    user_space->ptr_proc_curr = NULL;
    user_space->ptr_thread = NULL;
    user_space->pid_next = 1;
    user_space->tid_next = 1;
    user_space->internal_pool_base = ULIME_START + sizeof(user_space_t);
    user_space->internal_pool_size = ULIME_META_SIZE - sizeof(user_space_t);
    user_space->internal_pool_used = 0;
    user_space->user_space_base = 0x40000000;
    user_space->user_space_size = 0x00000000FFFFFFFF;
    user_space->user_space_used = user_space->user_space_base;
    user_space->hpr = hpr;
    return user_space;
}
user_space_proc_t *user_space_proc_create(user_space_t *user_space, u8 *name, u64 entry_point) {
    if (user_space->internal_pool_used + sizeof(user_space_proc_t) > user_space->internal_pool_size) {
        return NULL;
    }
    user_space_proc_t *proc = (user_space_proc_t*)(user_space->internal_pool_base + user_space->internal_pool_used);
    user_space->internal_pool_used += sizeof(user_space_proc_t);
    proc->user_space = user_space;
    memset(proc, 0, sizeof(user_space_proc_t));
    proc->pid = user_space->pid_next++;
    int i = 0;
    while (name[i] != '\0')
    {
        proc->name[i] = name[i];
        i++;
    }
    proc->name[i] = '\0';
    proc->state = PROC_CREATED;
    proc->entry_point = entry_point;
    proc->stack_base = user_space->user_space_used;
    proc->stack_size = 16 * 1024;
    user_space->user_space_used += proc->stack_size;
    proc->heap_base = user_space->user_space_used;
    proc->heap_size = 64 * 1024;
    user_space->user_space_used += proc->heap_size;
    proc->next = user_space->ptr_proc_list;
    if (user_space->ptr_proc_list) user_space->ptr_proc_list->prev = proc;
    user_space->ptr_proc_list = proc;
    return proc;
}
int user_space_proc_kill(user_space_t *user_space, u64 pid) {
    user_space_proc_t *proc = user_space->ptr_proc_list;
    while (proc) {
        if (proc->pid == pid) {
            proc->state = PROC_ZOMBIE;
            return 0;
        }
        proc = proc->next;
    }
    return 1;
}
int user_space_proc_mmap(user_space_t *user_space, user_space_proc_t *proc) {
    u64 phys_stack_len = proc->stack_size / PAGE_SIZE;
    u64 phys_stack_pos = physmem_alloc_to(phys_stack_len);
    if (!phys_stack_pos) {
        return 1;
    }
    u64 phys_heap_len  = proc->heap_size / PAGE_SIZE;
    u64 phys_heap_pos  = physmem_alloc_to(phys_heap_len);
    if (!phys_heap_pos) {
        return 1;
    }
    u64 kernel_start = 0x204000;
    u64 kernel_end = kernel_start + HEAP_SIZE;
    map_region(user_space->hpr, phys_stack_pos, proc->stack_base, proc->stack_size);
    map_region(user_space->hpr, phys_heap_pos, proc->heap_base, proc->heap_size);
    proc->phys_stack = phys_stack_pos;
    proc->phys_heap = phys_heap_pos;
    return 0;
}
void user_space_schedule(user_space_t *user_space) {
    if (!user_space->ptr_proc_list) return;
    if (!user_space->ptr_proc_curr) {
        user_space->ptr_proc_curr = user_space->ptr_proc_list;
    } else {
        user_space->ptr_proc_curr = user_space->ptr_proc_curr->next;
        if (!user_space->ptr_proc_curr) {
            user_space->ptr_proc_curr = user_space->ptr_proc_list;
        }
    }
    if (user_space->ptr_proc_curr->state == PROC_READY) {
        user_space->ptr_proc_curr->state = PROC_RUNNING;
    }
}
void user_space_proc_list(user_space_t *user_space) {
    BOOTUP_PRINTF("Process List:\n");
    BOOTUP_PRINTF("PID\tState\tName\t\tEntry Point\n");
    BOOTUP_PRINTF("---\t-----\t----\t\t-----------\n");
    user_space_proc_t *current = user_space->ptr_proc_list;
    while (current) {
        const char *state_str = "UNKNOWN";
        switch (current->state) {
            case PROC_CREATED: state_str = "CREATED"; break;
            case PROC_READY:   state_str = "READY"; break;
            case PROC_RUNNING: state_str = "RUNNING"; break;
            case PROC_BLOCKED: state_str = "BLOCKED"; break;
            case PROC_ZOMBIE:  state_str = "ZOMBIE"; break;
        }
        BOOTUP_PRINTF("%lu\t%s\t%s\t\t0x%lX\n",
               current->pid, state_str, current->name, current->entry_point);
        current = current->next;
    }
}
int user_space_load_program(user_space_proc_t *proc, u8 *code, u64 code_size) {
    if (code_size > proc->heap_size) {
        BOOTUP_PRINTF("Program too large for process heap\n");
        return 1;
    }
    memcpy((void*)proc->heap_base, code, code_size);
    proc->entry_point = proc->heap_base;
    BOOTUP_PRINTF("Loaded program (%lu bytes) into process %s at 0x%lX\n",
                    code_size, proc->name, proc->entry_point);
    return 0;
}
u64 sys_exit(user_space_proc_t *proc, u64 exit_code, u64 arg2, u64 arg3) {
    BOOTUP_PRINTF("Process %s (PID %lu) exiting with code %lu\n",
           proc->name, proc->pid, exit_code);
    proc->state = PROC_ZOMBIE;
    return 0;
}
u64 sys_write(user_space_proc_t *proc, u64 fd, u64 buf, u64 count) {
    u8 *str = (u8 *)buf;
    for (u64 i = 0; i < count; i++) {
        count ++;
    }
    gsession_put_at_string_dummy(
        proc->user_space->graphics_manager->workspaces[0]->sessions[0],
        str,
        0, 0,
        0x00FFFFFF
    );
    return count;
}
void user_space_init_syscalls(user_space_t *user_space) {
    memset(user_space->syscalls, 0, sizeof(user_space->syscalls));
    user_space->syscalls[SYS_EXIT]   = sys_exit;
    user_space->syscalls[SYS_WRITE]  = sys_write;
}