#ifndef GRAPHICS_MANAGER_H
#define GRAPHICS_MANAGER_H
#include <outputs/types.h>
#include "../mem.h"
#include "../heap/heap.h"
typedef struct gbox {
    u64 x;
    u64 y;
    u64 width;
    u64 height;
} gbox_t;
typedef struct gsession {
    struct gworkspace *gworkspace;
    struct keyboardrb *kbrb;
    u8 name[64];
    gbox_t box;
} gsession_t;
#define GSESSION_LEN_MAX 16
#define SESSION_NAME_MAX 64
#define GWORKSPACE_NAME_MAX 64
#define GWORKSPACE_LEN_MAX 64
typedef struct gworkspace {
    struct graphics_manager *graphics_manager;
    u8 name[64];
    u64 pos_x;
    u64 pos_y;
    u64 gap_x;
    gsession_t **sessions;
    u64 sessions_len;
    u64 sessions_total;
    u64 session_active;
} gworkspace_t;
typedef struct graphics_manager {
    graphics_response_t glres;
    u32 *framebuffer;
    u64 framebuffer_len;
    gworkspace_t **workspaces;
    u64 workspaces_len;
    u64 workspaces_total;
    u64 workspace_active;
    u64 dirty_boxes_len;
    gbox_t *dirty_boxes;
    heap_block_t *start_heap;
    u64 total_heap;
    u64 used_heap;
} graphics_manager_t;
graphics_manager_t *graphics_manager_init(graphics_response_t *gr, u64 *ptr, u64 size);
void graphics_manager_commit(graphics_manager_t *graphics_manager);
u64 *graphics_manager_create(graphics_manager_t *graphics_manager, u64 size);
u64 *graphics_manager_alloc(graphics_manager_t *graphics_manager, u64 size, u64 count);
void graphics_manager_free(graphics_manager_t *graphics_manager, u64 *ptr);
u64 graphics_manager_get_total_size(graphics_manager_t *graphics_manager);
u64 graphics_manager_get_used_size(graphics_manager_t *graphics_manager);
u64 graphics_manager_get_free_size(graphics_manager_t *graphics_manager);
int gworkspace_init(graphics_manager_t *graphics_manager, u8 *name, u64 pos);
int gworkspace_posx_get(gworkspace_t *gworkspace, u64 *out);
gworkspace_t *gworkspace_get_name(graphics_manager_t *graphics_manager, u8 *workspace_name);
gworkspace_t *gworkspace_get_pos(graphics_manager_t *graphics_manager, u64 pos);
gsession_t *gsession_init(graphics_manager_t *graphics_manager, u8 *name, u64 width);
int gsession_attach(graphics_manager_t *graphics_manager, gsession_t *gsession, u8 *workspace_name);
int gsession_detach(gworkspace_t *gworkspace, gsession_t *gsession);
int gsession_reattach(graphics_manager_t *graphics_manager, gsession_t *gsession, u8 *workspace_name);
void gsession_clear(gsession_t *gsession, u32 color);
void gsession_put_at_string_dummy(gsession_t *gsession, u8 *string, u32 x, u32 y, u32 color);
void gsession_put_at_char_dummy(gsession_t *gsession, u8 c, u32 x, u32 y, u32 color);
typedef struct key_event {
    u8 scancode;
    u8 modifiers;
    u8 pressed;
} key_event_t;
typedef struct keyboardrb {
    key_event_t *buf;
    u32 len;
    u32 head;
    u32 tail;
    u32 count;
} keyboardrb_t;
int keyboard_put(keyboardrb_t *kbrb, key_event_t event);
int keyboard_next(keyboardrb_t *kbrb, key_event_t *out);
u8 keyboard_event_to_char(key_event_t event);
keyboardrb_t *graphics_manager_keyboard_init(graphics_manager_t *graphics_manager, u64 count);
void graphics_manager_keyboard_handler(graphics_manager_t *graphics_manager);
void graphics_manager_disable_double_buffering(void);
extern u32 font_scale;
#endif