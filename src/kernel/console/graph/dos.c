#include "dos.h"
#include "uno.h"
#include <kernel/graph/graphics.h>
#include <kernel/graph/fm.h>
void console_window_init(void) {
}
void console_window_clear(u32 color)
{
    u32 fb_w = get_fb_width();
    u32 fb_h = get_fb_height();
    u32 banner_h = banner_get_height();
    draw_rect(0, banner_h, fb_w, fb_h - banner_h, color);
    banner_draw();
    cursor_x = CONSOLE_PADDING_X;
    cursor_y = banner_h;
}
u32 console_window_get_start_y(void) {
    return banner_get_height();
}
u32 console_window_get_max_y(void) {
    return get_fb_height();
}
void console_window_check_scroll(void)
{
}
void console_window_update_layout(void)
{
    u32 banner_h = banner_get_height();
    if (cursor_y < banner_h) {
        cursor_y = banner_h;
    }
}
void console_window_scroll_lines(u32 lines)
{
    if (lines == 0) return;
    u32 char_height = fm_get_char_height() * font_scale;
    u32 banner_h = banner_get_height();
    scroll_up(lines);
    u32 scroll_pixels = lines * char_height;
    if (cursor_y >= scroll_pixels) {
        cursor_y -= scroll_pixels;
    } else {
        cursor_y = banner_h;
    }
    if (graphics_is_double_buffering_enabled()) {
        graphics_swap_buffers();
    }
}
u32 console_window_get_visible_lines(void)
{
    u32 char_height = fm_get_char_height() * font_scale;
    u32 fb_h = get_fb_height();
    u32 banner_h = banner_get_height();
    u32 available_height = fb_h - banner_h;
    return available_height / char_height;
}
int console_window_needs_scroll(void)
{
    u32 char_height = fm_get_char_height() * font_scale;
    u32 fb_h = get_fb_height();
    return (cursor_y + char_height > fb_h) ? 1 : 0;
}