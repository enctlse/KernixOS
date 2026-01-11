#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <outputs/types.h>
#include <limine/limine.h>
#include <config/boot.h>
#include <ui/theme/colors.h>
#include <kernel/graph/theme.h>
#include <string/string.h>
extern u32 *framebuffer;
extern u32 *backbuffer;
extern u32 fb_width;
extern u32 fb_height;
extern u32 fb_pitch;
extern u32 cursor_x;
extern u32 cursor_y;
extern u32 font_scale;
extern u32 font_scale;
void graphics_init(struct limine_framebuffer *fb, void *kernel_memory_ptr);
void putpixel(u32 x, u32 y, u32 color);
u32 getpixel(u32 x, u32 y);
u32 get_fb_width(void);
u32 get_fb_height(void);
u32* get_framebuffer(void);
u32* get_backbuffer(void);
u32 get_fb_pitch(void);
void reset_cursor(void);
void clear(u32 color);
void scroll_up(u32 lines);
void set_font_scale(u32 scale);
u32 get_font_scale(void);
void draw_rect(u32 x, u32 y, u32 width, u32 height, u32 color);
void draw_circle(u32 cx, u32 cy, u32 radius, u32 color);
void draw_line(u32 x0, u32 y0, u32 x1, u32 y1, u32 color);
void graphics_set_font_scale(u32 scale);
void graphics_disable_double_buffering(void);
u32 graphics_get_font_scale(void);
void graphics_enable_double_buffering(void);
void graphics_swap_buffers(void);
int graphics_is_double_buffering_enabled(void);
void putpixels_horizontal(u32 x, u32 y, u32 width, u32 color);
void gui_draw_text_line_fast(const char* str, u32 x, u32 y, u32 color);
#endif