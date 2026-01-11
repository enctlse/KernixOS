#ifndef MOUSE_H
#define MOUSE_H
#include <outputs/types.h>
#include <kernel/module/module.h>
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t* pixels;
} cursor_image_t;
#define MOUSE_BUTTON_LEFT   0x01
#define MOUSE_BUTTON_RIGHT  0x02
#define MOUSE_BUTTON_MIDDLE 0x04
typedef void (*mouse_callback_t)(int32_t x, int32_t y, uint8_t buttons);
int mouse_init();
void mouse_get_position(int32_t *x, int32_t *y);
void mouse_set_position(int32_t x, int32_t y);
uint8_t mouse_get_buttons();
int8_t mouse_get_scroll_delta();
int mouse_has_scroll_wheel();
void mouse_set_callback(mouse_callback_t callback);
void mouse_unregister_callback();
void mouse_draw_cursor();
int mouse_is_initialized();
int mouse_get_interrupt_count();
void mouse_enable_test_mode();
int mouse_cursor_needs_update();
void mouse_clear_cursor_redraw_flag();
cursor_image_t* load_cursor_from_file(const char* path);
void mouse_force_update();
void mouse_reload_cursor();
void mouse_set_sensitivity(int divider);
int mouse_get_sensitivity();
extern driver_module mouse_module;
#endif