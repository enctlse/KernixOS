#include "physmem.h"
#include <limine/limine.h>
#include <kernel/interrupts/panic/panic.h>
#include <drivers/memory/mem.h>
#include <kernel/communication/serial.h>
#include <config/boot.h>
static struct physmem_pageframe *physmem_pageframes = NULL;
static u64 physmem_total = 0;
static u8 *bitmap        = NULL;
static u64 bitmap_size   = 0;
static u64 physmem_addr_highest = 0;
static inline int bitmap_test(u64 idx) {
    return (bitmap[idx >> 3] >> (idx & 7)) & 1;
}
static inline void bitmap_set(u64 idx) {
    bitmap[idx >> 3] |= (1 << (idx & 7));
}
static inline void bitmap_clear(u64 idx) {
    bitmap[idx >> 3] &= ~(1 << (idx & 7));
}
static void physmem_addr_mark(limine_memmap_response_t *mpr) {
    if (!physmem_pageframes) {
        initiate_panic("PHYSMEM ADDR MARK PHYSMEM PAGEFRAMES NULL");
        return;
    }
    for (u64 i = 0; i < mpr->entry_count; i++) {
        struct limine_memmap_entry *entry = mpr->entries[i];
        u64 frame_start = entry->base / PAGE_SIZE;
        if (frame_start >= physmem_total) continue;
        u64 frame_end = (entry->base + entry->length) / PAGE_SIZE;
        if (frame_end > physmem_total) frame_end = physmem_total;
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            for (u64 frame = frame_start; frame < frame_end; frame++) {
                bitmap_clear(frame);
                physmem_pageframes[frame].rc = 0;
                physmem_pageframes[frame].flags = FRAME_FREE;
            }
        } else {
            for (u64 frame = frame_start; frame < frame_end; frame++) {
                bitmap_set(frame);
                physmem_pageframes[frame].rc = 1;
                physmem_pageframes[frame].flags = FRAME_USED | FRAME_KERNEL;
            }
        }
    }
}
void physmem_addr_mark_used(u64 physmem_addr, u64 count) {
    if (!physmem_pageframes) {
        initiate_panic("PHYSMEM ADDR MARK USED PHYSMEM PAGEFRAMES NULL");
        return;
    }
    u64 frame_start = physmem_addr / PAGE_SIZE;
    u64 frame_end = frame_start + count;
    if (frame_end > physmem_total) frame_end = physmem_total;
    for (u64 frame = frame_start; frame < frame_end; frame++) {
        bitmap_set(frame);
        physmem_pageframes[frame].rc = 1;
        physmem_pageframes[frame].flags = FRAME_USED;
    }
}
void physmem_addr_mark_free(u64 physmem_addr, u64 count) {
    if (!physmem_pageframes) {
        initiate_panic("PHYSMEM ADDR MARK FREE PHYSMEM PAGEFRAMES NULL");
        return;
    }
    u64 frame_start = physmem_addr / PAGE_SIZE;
    u64 frame_end = frame_start + count;
    if (frame_end > physmem_total) frame_end = physmem_total;
    for (u64 frame = frame_start; frame < frame_end; frame++) {
        bitmap_clear(frame);
        physmem_pageframes[frame].rc = 0;
        physmem_pageframes[frame].flags = FRAME_FREE;
    }
}
void *physmem_addr_get_tracking(
    limine_memmap_response_t *mpr,
    limine_hhdm_response_t *hpr,
    u64 size
) {
    if (!mpr) initiate_panic("PHYSMEM ADDR GET LIMINE MEMMAP EQ NULL\n");
    if (!hpr) initiate_panic("PHYSMEM ADDR GET LIMINE HHDM EQ NULL\n");
    for (u64 i = 0; i < mpr->entry_count; i++) {
        struct limine_memmap_entry *entry = mpr->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE &&
            entry->length >= size &&
            entry->base >= 0x1000000) {
            return (void *)entry->base;
        }
    }
    for (u64 i = 0; i < mpr->entry_count; i++) {
        struct limine_memmap_entry *entry = mpr->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE &&
            entry->length >= size &&
            entry->base >= 0x100000) {
            return (void *)entry->base;
        }
    }
    for (u64 i = 0; i < mpr->entry_count; i++) {
        struct limine_memmap_entry *entry = mpr->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE &&
            entry->length >= size) {
            return (void *)entry->base;
        }
    }
    return NULL;
}
u64 used_bytes_to_frame_count(u64 size)  {
    return (size + PAGE_SIZE - 1) / PAGE_SIZE;
}
void physmem_init(limine_memmap_response_t *mpr, limine_hhdm_response_t *hpr) {
    physmem_addr_highest = 0;
    for (u64 i = 0; i < mpr->entry_count; i++) {
        struct limine_memmap_entry *entry = mpr->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE ||
            entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
            u64 end_addr = entry->base + entry->length;
            if (end_addr > physmem_addr_highest) {
                physmem_addr_highest = end_addr;
            }
        }
    }
    physmem_total = physmem_addr_highest / PAGE_SIZE;
    u64 physmem_size = physmem_total * sizeof(physmem_pageframe_t);
    bitmap_size = (physmem_total + 7) / 8;
    void *physmem_pageframe_addr = physmem_addr_get_tracking(mpr, hpr, physmem_size + bitmap_size);
    if (!physmem_pageframe_addr) initiate_panic("No usable memory found for physmem_pageframes");
    void *physmem_pageframes_vaddr = (void *)((u64)physmem_pageframe_addr + hpr->offset);
    physmem_pageframes = (physmem_pageframe_t *)physmem_pageframes_vaddr;
    void *bitmap_addr = (void *)((u64)physmem_pageframe_addr + physmem_size);
    bitmap = (u8 *)((u64)bitmap_addr + hpr->offset);
    memset(bitmap, 0xFF, bitmap_size);
    physmem_addr_mark(mpr);
    u64 to_used = used_bytes_to_frame_count(physmem_size + bitmap_size);
    physmem_addr_mark_used((u64)physmem_pageframe_addr, to_used);
}
u64 physmem_get_total() {
    return physmem_total;
}
u64 physmem_alloc_to(u64 count) {
    if (count == 0) return 0;
    u64 consecutive = 0;
    u64 start_frame = 0;
    u8  all_free = 0;
    u64 start_search = 512;
    for (u64 frame = start_search; frame < physmem_total; frame++) {
        if (!bitmap_test(frame)) {
            if (consecutive == 0) {
                start_frame = frame;
            }
            consecutive++;
            if (consecutive >= count) {
                all_free = 1;
                break;
            }
        } else {
            consecutive = 0;
        }
    }
    if (all_free) {
        u64 physmem_addr = start_frame * PAGE_SIZE;
        physmem_addr_mark_used(physmem_addr, count);
        return physmem_addr;
    }
    return 0;
}
void physmem_free_to(u64 physmem_addr, u64 count) {
    if (count == 0) return;
    physmem_addr_mark_free(physmem_addr, count );
}
u64 physmem_free_get(void) {
    u64 count = 0;
    for (u64 frame = 0; frame < physmem_total; frame++) {
        if (!bitmap_test(frame)) {
            count++;
        }
    }
    return count;
}