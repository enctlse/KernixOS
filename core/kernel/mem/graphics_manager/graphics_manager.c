#include "graphics_manager.h"
#include <kernel/interrupts/panic/panic.h>
#include "../heap/heap.h"
#include <string/string.h>
#include <drivers/memory/mem.h>
#include <kernel/communication/serial.h>
#include <config/boot.h>
#include <kernel/include/io.h>
static int double_buffering_enabled = 0;
graphics_manager_t *graphics_manager_init(graphics_response_t *gr, u64 *ptr, u64 size) {
    if (!gr) {
        SYSTEM_PRINTF("ERROR: Invalid graphics_manager init graphics_manager response ptr is null");
        initiate_panic( "ERROR: Invalid graphics_manager init graphics_manager response ptr is null");
    }
    if (!ptr) {
        SYSTEM_PRINTF("ERROR: Invalid graphics_manager init ptr to memory is null");
        initiate_panic( "ERROR: Invalid graphics_manager init ptr to memory is null");
    }
    u64 framebuffer_len = sizeof(u32) * gr->width * gr->height;
    if (size < sizeof(heap_block_t) + GRAPHICS_SIZE_META + framebuffer_len) {
        SYSTEM_PRINTF("ERROR: Invalid graphics_manager init size lt needed");
        initiate_panic( "ERROR: Invalid graphics_manager init size lt needed");
    }
    graphics_manager_t *graphics_manager = (graphics_manager_t *)ptr;
    graphics_manager->glres.start_framebuffer = gr->start_framebuffer;
    graphics_manager->glres.width = gr->width;
    graphics_manager->glres.height = gr->height;
    graphics_manager->glres.pitch = gr->pitch;
    graphics_manager->total_heap = GRAPHICS_HEAP_SIZE - sizeof(heap_block_t) - framebuffer_len;
    graphics_manager->used_heap = 0;
    graphics_manager->start_heap = (heap_block_t *)((u64)graphics_manager + GRAPHICS_SIZE_META + framebuffer_len);
    graphics_manager->start_heap->magic = BLOCK_MAGIC;
    graphics_manager->start_heap->size = graphics_manager->total_heap;
    graphics_manager->start_heap->next = NULL;
    graphics_manager->start_heap->prev = NULL;
    graphics_manager->start_heap->used = 0;
    graphics_manager->framebuffer_len = framebuffer_len;
    graphics_manager->framebuffer = (u32 *)graphics_manager_create(graphics_manager, framebuffer_len);
    if (!graphics_manager->framebuffer) {
        SYSTEM_PRINTF("ERROR: Invalid graphics_manager init framebuffer is not initialized");
        initiate_panic( "ERROR: Invalid graphics_manager init framebuffer is not initialized");
    }
    u64 workspaces_total = 1;
    graphics_manager->workspaces = (gworkspace_t **) graphics_manager_alloc(graphics_manager, sizeof(gworkspace_t), workspaces_total);
    if (!graphics_manager->workspaces) {
        SYSTEM_PRINTF("ERROR: Invalid graphics_manager init workspaces is not initialized");
        initiate_panic( "ERROR: Invalid graphics_manager init workspaces is not initialized");
    }
    graphics_manager->workspaces_len = 0;
    graphics_manager->workspaces_total = workspaces_total ;
    graphics_manager->dirty_boxes = (gbox_t *) graphics_manager_alloc(graphics_manager, sizeof(gbox_t), 256);
    graphics_manager->dirty_boxes_len = 0;
    return graphics_manager;
}
void graphics_manager_dirtybox_append(graphics_manager_t *graphics_manager, u64 x, u64 y, u64 width, u64 height) {
    gbox_t db = {
        .x = x,
        .y = y,
        .width = width,
        .height = height,
    };
    graphics_manager->dirty_boxes[graphics_manager->dirty_boxes_len] = db;
    graphics_manager->dirty_boxes_len++;
}
void graphics_manager_commit(graphics_manager_t *graphics_manager) {
    u32 *fb = (u32 *)graphics_manager->glres.start_framebuffer;
    u64 pitch_pixels = graphics_manager->glres.pitch / sizeof(u32);
    for (int i = 0; i < graphics_manager->dirty_boxes_len; i++) {
        gbox_t db = graphics_manager->dirty_boxes[i];
        for (int y = 0; y < db.height; y++) {
            u32 src_offset = (db.y + y) * graphics_manager->glres.width + db.x;
            u32 dst_offset = (db.y + y) * pitch_pixels + db.x;
            memset(&fb[dst_offset], 0, db.width * sizeof(u32));
            memcpy(&fb[dst_offset], &graphics_manager->framebuffer[src_offset], db.width * sizeof(u32));
        }
    }
    graphics_manager->dirty_boxes_len = 0;
}
u64 *graphics_manager_create(graphics_manager_t *graphics_manager, u64 size) {
    if (!graphics_manager || size == 0) return NULL;
    u64 *ptr = malloc(graphics_manager->start_heap, size);
    if (!ptr) return NULL;
    heap_block_t *block = (heap_block_t*)((u8*)ptr - sizeof(heap_block_t));
    graphics_manager->used_heap += block->size + sizeof(heap_block_t);
    return ptr;
}
u64 *graphics_manager_alloc(graphics_manager_t *graphics_manager, u64 size, u64 count) {
    if (!graphics_manager || size == 0) return NULL;
    u64 total = count * size;
    u64 *ptr = malloc(graphics_manager->start_heap, total);
    if (!ptr) return NULL;
    memset(ptr, 0, total);
    heap_block_t *block = (heap_block_t*)((u8*)ptr - sizeof(heap_block_t));
    graphics_manager->used_heap += block->size + sizeof(heap_block_t);
    return ptr;
}
void graphics_manager_free(graphics_manager_t *graphics_manager, u64 *ptr) {
    if (!graphics_manager || !ptr) return;
    heap_block_t *block = (heap_block_t*)((u8*)ptr - sizeof(heap_block_t));
    graphics_manager->used_heap -= block->size;
    int merged = free(ptr);
    graphics_manager->used_heap -= sizeof(heap_block_t) * merged;
}
u64 graphics_manager_get_total_size(graphics_manager_t *graphics_manager) {
    return graphics_manager->total_heap;
}
u64 graphics_manager_get_used_size(graphics_manager_t *graphics_manager) {
    return graphics_manager->used_heap;
}
u64 graphics_manager_get_free_size(graphics_manager_t *graphics_manager) {
    return graphics_manager->total_heap - graphics_manager->used_heap;
}
void graphics_manager_keyboard_handler(graphics_manager_t *graphics_manager) {
    u8 last = 0;
    while (1) {
        if ((inb(0x64) & 1) == 0) continue;
        u8 scancode = inb(0x60);
        if (scancode == last) continue;
        last = scancode;
        key_event_t event = {
            .scancode = scancode & 0x7F,
            .pressed = !(scancode & 0x80),
            .modifiers = 0,
        };
        gworkspace_t *ws = graphics_manager->workspaces[graphics_manager->workspace_active];
        gsession_t *s = ws->sessions[ws->session_active];
        keyboard_put(s->kbrb, event);
        u8 c = keyboard_event_to_char(event);
        gsession_clear(s, 0x00000000);
        graphics_manager_dirtybox_append(graphics_manager, s->box.x, s->box.y, 8, 8);
        gsession_put_at_char_dummy(s, c, 0, 0, 0x00FFFFFF);
        graphics_manager_commit(graphics_manager);
    }
}
void graphics_manager_disable_double_buffering(void) {
    double_buffering_enabled = 0;
}