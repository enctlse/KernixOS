#include "kernel_memory.h"
#include "../heap/heap.h"
#include "../slab/slab.h"
#include "../paging/paging.h"
#include <kernel/interrupts/panic/panic.h>
#include <drivers/memory/mem.h>
#include <kernel/communication/serial.h>

kernel_memory_t *global_kernel_memory = NULL;
kernel_memory_t *kernel_memory_init(u64 *ptr, u64 size) {
    if (!ptr || size < sizeof(heap_block_t)) {
        initiate_panic("ERROR: Invalid kernel_memory initializing");
    }
    kernel_memory_t *kernel_memory = (kernel_memory_t *)ptr;
    kernel_memory->start_slab_meta = (u64 *)((u8 *)kernel_memory + KERNEL_MEMORY_OFFSET_SLAB_META);
    kernel_memory->start_slab_data = (u64 *)((u8 *)kernel_memory + KERNEL_MEMORY_OFFSET_SLAB_DATA);
    kernel_memory->start_io        = (u64 *)((u8 *)kernel_memory + KERNEL_MEMORY_OFFSET_IO);
    kernel_memory->start_dma       = (u64 *)((u8 *)kernel_memory + KERNEL_MEMORY_OFFSET_DMA);
    kernel_memory->start_heap      = (heap_block_t *)((u8 *)kernel_memory + KERNEL_MEMORY_OFFSET_HEAP);
    kernel_memory->total_heap      = KERNEL_MEMORY_SIZE_HEAP - sizeof(heap_block_t);
    kernel_memory->used_heap       = 0;
    kernel_memory->start_heap->magic = BLOCK_MAGIC;
    kernel_memory->start_heap->size = kernel_memory->total_heap;
    kernel_memory->start_heap->next = NULL;
    kernel_memory->start_heap->prev = NULL;
    kernel_memory->start_heap->used = 0;
    slab_init(&kernel_memory->slab_allocator, kernel_memory->start_slab_meta, kernel_memory->start_slab_data);
    return kernel_memory;
}
void kernel_memory_setup_slab(kernel_memory_t *kernel_memory) {
}
u64 *kernel_memory_create(kernel_memory_t *kernel_memory, u64 size) {
    if (!kernel_memory || size == 0) return NULL;
    u64 *ptr = malloc(kernel_memory->start_heap, size);
    if (!ptr) return NULL;
    heap_block_t *block = (heap_block_t*)((u8*)ptr - sizeof(heap_block_t));
    kernel_memory->used_heap += block->size + sizeof(heap_block_t);
    return ptr;
}
u64 *kernel_memory_alloc(kernel_memory_t *kernel_memory, u64 size, u64 count) {
    if (!kernel_memory || size == 0) return NULL;
    u64 total = count * size;
    u64 *ptr = malloc(kernel_memory->start_heap, total);
    if (!ptr) return NULL;
    memset(ptr, 0, total);
    heap_block_t *block = (heap_block_t*)((u8*)ptr - sizeof(heap_block_t));
    kernel_memory->used_heap += block->size + sizeof(heap_block_t);
    return ptr;
}
void kernel_memory_free(kernel_memory_t *kernel_memory, u64 *ptr) {
    if (!kernel_memory || !ptr) return;
    heap_block_t *block = (heap_block_t*)((u8*)ptr - sizeof(heap_block_t));
    kernel_memory->used_heap -= block->size;
    int merged = free(ptr);
    kernel_memory->used_heap -= sizeof(heap_block_t) * merged;
}
u64 kernel_memory_get_total_size(kernel_memory_t *kernel_memory) {
    return kernel_memory->total_heap;
}
u64 kernel_memory_get_used_size(kernel_memory_t *kernel_memory) {
    return kernel_memory->used_heap;
}
u64 kernel_memory_get_free_size(kernel_memory_t *kernel_memory) {
    return kernel_memory->total_heap - kernel_memory->used_heap;
}