#ifndef MEM_H
#define MEM_H
#include <outputs/types.h>
#include <limine/limine.h>
typedef struct limine_memmap_response limine_memmap_response_t;
typedef struct limine_hhdm_response limine_hhdm_response_t;
typedef struct limine_framebuffer_response limine_framebuffer_response_t;
typedef struct limine_framebuffer limine_framebuffer_t;
#define HEAP_START 0xFFFF800000000000
#define HEAP_SIZE 1024 * 1024 * 32
#define GRAPHICS_START (HEAP_START + HEAP_SIZE)
#define GRAPHICS_SIZE 1024 * 1024 * 32
#define ULIME_START (GRAPHICS_START + GRAPHICS_SIZE)
#define ULIME_META_SIZE (1024 * 1024 * 2)
#define KERNEL_MEMORY_SIZE_SLAB   (1024 * 1024 * 8)
#define KERNEL_MEMORY_SIZE_IO     (1024 * 1024 * 4)
#define KERNEL_MEMORY_SIZE_DMA    (1024 * 1024 * 2)
#define KERNEL_MEMORY_SIZE_NOTHEAP (PAGE_SIZE + PAGE_SIZE + KERNEL_MEMORY_SIZE_SLAB + KERNEL_MEMORY_SIZE_IO + KERNEL_MEMORY_SIZE_DMA)
#define KERNEL_MEMORY_SIZE_HEAP HEAP_SIZE - KERNEL_MEMORY_SIZE_NOTHEAP
#define KERNEL_MEMORY_OFFSET_SLAB_META PAGE_SIZE
#define KERNEL_MEMORY_OFFSET_SLAB_DATA KERNEL_MEMORY_OFFSET_SLAB_META + PAGE_SIZE
#define KERNEL_MEMORY_OFFSET_IO        KERNEL_MEMORY_OFFSET_SLAB_DATA + KERNEL_MEMORY_SIZE_SLAB
#define KERNEL_MEMORY_OFFSET_DMA       KERNEL_MEMORY_OFFSET_IO        + KERNEL_MEMORY_SIZE_IO
#define KERNEL_MEMORY_OFFSET_HEAP      KERNEL_MEMORY_OFFSET_DMA       + KERNEL_MEMORY_SIZE_DMA
#define GRAPHICS_SIZE_META (PAGE_SIZE * 256)
#define GRAPHICS_HEAP_SIZE (GRAPHICS_SIZE - GRAPHICS_SIZE_META)
#define PAGE_SIZE 4096
#define FRAME_FREE       0x00
#define FRAME_USED       0x01
#define FRAME_KERNEL     0x02
#define FRAME_USER       0x04
#define FRAME_DMA        0x08
#define FRAME_SHARED     0x10
#define FRAME_COW        0x20
#define FRAME_CACHE      0x40
#define FRAME_GUARD      0x80
typedef struct graphics_request {
    limine_framebuffer_response_t *fbr;
    u64 virt;
    u64 size;
} graphics_request_t;
typedef struct graphics_response {
    u64 *start_framebuffer;
    u64 width;
    u64 height;
    u64 pitch;
    u16 bpp;
    u16 memory_model;
    u8 red_mask_size;
    u8 red_mask_shift;
    u8 green_mask_size;
    u8 green_mask_shift;
    u8 blue_mask_size;
    u8 blue_mask_shift;
} graphics_response_t;
typedef struct kernel_memory_request {
    limine_hhdm_response_t *hpr;
    u64 phys;
    u64 virt;
    u64 size;
} kernel_memory_request_t;
typedef struct kernel_memory_response {
    u64 *ptr;
    u64 size;
} kernel_memory_response_t;
#endif