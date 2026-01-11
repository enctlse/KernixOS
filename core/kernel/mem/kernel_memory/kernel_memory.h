#ifndef KERNEL_MEMORY_H
#define KERNEL_MEMORY_H
#include "../mem.h"
#include <outputs/types.h>
#include "../slab/slab.h"
#include "../heap/heap.h"
typedef struct kernel_memory {
    u64 *start_slab_meta;
    u64 *start_slab_data;
    u64 *start_io;
    u64 *start_dma;
    heap_block_t *start_heap;
    slab_allocator_t slab_allocator;
    u64 total_heap;
    u64 used_heap;
} kernel_memory_t;
kernel_memory_t *kernel_memory_init(u64 *ptr, u64 size);
void kernel_memory_setup_slab(kernel_memory_t *kernel_memory);
void *kernel_memory_alloc_slab(kernel_memory_t *kernel_memory, u64 size);
void kernel_memory_free_slab(kernel_memory_t *kernel_memory, u64 *ptr);
u64 *kernel_memory_create(kernel_memory_t *kernel_memory, u64 size);
u64 *kernel_memory_alloc(kernel_memory_t *kernel_memory, u64 size, u64 count);
void kernel_memory_free(kernel_memory_t *kernel_memory, u64 *ptr);
u64  kernel_memory_get_total_size(kernel_memory_t *kernel_memory);
u64  kernel_memory_get_used_size(kernel_memory_t *kernel_memory);
u64  kernel_memory_get_free_size(kernel_memory_t *kernel_memory);
#endif