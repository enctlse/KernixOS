#include "visual.h"
#include <ui/fonts/font_8x16.h>
#include <kernel/communication/serial.h>
#include <kernel/mem/kernel_memory/kernel_memory.h>
#include <drivers/memory/mem.h>
u32 *framebuffer = NULL;
u32 *backbuffer = NULL;
static int double_buffering_enabled = 0;
static kernel_memory_t *graphics_kernel_memory = NULL;
u32 fb_width = 0;
u32 fb_height = 0;
u32 fb_pitch = 0;
u32 cursor_x = 0;
u32 cursor_y = 0;
u32 font_scale = 1;
#define CHAR_WIDTH    8
#define CHAR_HEIGHT   16
#define CONSOLE_BG_COLOR  0x00000000
static inline u32 get_stride(void) {
    return fb_pitch / 4;
}
static inline u32* get_active_buffer(void) {
    return double_buffering_enabled ? backbuffer : framebuffer;
}
void graphics_init(struct limine_framebuffer *fb, void *kernel_memory_ptr)
{
    framebuffer = (u32 *)fb->address;
    fb_width = fb->width;
    fb_height = fb->height;
    fb_pitch = fb->pitch;
    graphics_kernel_memory = (kernel_memory_t*)kernel_memory_ptr;
    cursor_x = 0;
    cursor_y = 0;
    font_scale = 1;
    serial_printf("[GFX] Init: %ux%u pitch=%u addr=%p\n", 
                  fb_width, fb_height, fb_pitch, framebuffer);
}
void putpixel(u32 x, u32 y, u32 color)
{
    if (x >= fb_width || y >= fb_height) return;
    u32 stride = get_stride();
    u32* buf = get_active_buffer();
    buf[y * stride + x] = color;
}
u32 getpixel(u32 x, u32 y) {
    if (x >= fb_width || y >= fb_height) return 0;
    u32 stride = get_stride();
    u32* buf = get_active_buffer();
    return buf[y * stride + x];
}
void clear(u32 color)
{
    u32* buf = get_active_buffer();
    size_t total_pixels = (size_t)fb_height * get_stride();
    if (color == 0) {
        memset(buf, 0, total_pixels * 4);
    } else {
        u32* p = buf;
        size_t blocks = total_pixels / 8;
        size_t remainder = total_pixels % 8;
        while (blocks--) {
            p[0] = color; p[1] = color; p[2] = color; p[3] = color;
            p[4] = color; p[5] = color; p[6] = color; p[7] = color;
            p += 8;
        }
        while (remainder--) *p++ = color;
    }
    cursor_x = 0;
    cursor_y = 0;
}
void scroll_up(u32 lines)
{
    if (lines == 0) return;
    u32 char_h = CHAR_HEIGHT * font_scale;
    u32 pixels = lines * char_h;
    if (pixels >= fb_height) {
        clear(CONSOLE_BG_COLOR);
        cursor_y = 0;
        return;
    }
    u32* buf = get_active_buffer();
    if (!buf) return;
    u32 stride = get_stride();
    for (u32 y = 0; y < fb_height - pixels; y++) {
        u32* src = buf + (y + pixels) * stride;
        u32* dst = buf + y * stride;
        memcpy(dst, src, fb_pitch);
    }
    for (u32 y = fb_height - pixels; y < fb_height; y++) {
        u32* dst = buf + y * stride;
        memset(dst, 0, fb_pitch);
    }
    if (double_buffering_enabled) {
        graphics_swap_buffers();
    }
}
void graphics_enable_double_buffering(void)
{
    if (double_buffering_enabled) return;
    if (!graphics_kernel_memory || fb_width == 0 || fb_height == 0) return;
    u64 size_needed = (u64)fb_height * fb_pitch;
    backbuffer = (u32*)kernel_memory_alloc(graphics_kernel_memory, size_needed, 1);
    if (!backbuffer) {
        serial_printf("[GFX] ERROR: Backbuffer alloc failed (need %lu bytes)\n", size_needed);
        return;
    }
    memcpy(backbuffer, framebuffer, size_needed);
    double_buffering_enabled = 1;
    serial_printf("[GFX] Double buffering enabled (%lu KB)\n", size_needed / 1024);
}
void graphics_swap_buffers(void)
{
    if (!double_buffering_enabled || !backbuffer) return;
    memcpy(framebuffer, backbuffer, (size_t)fb_height * fb_pitch);
}
void graphics_disable_double_buffering(void)
{
    double_buffering_enabled = 0;
}
int graphics_is_double_buffering_enabled(void)
{
    return double_buffering_enabled;
}
void gui_draw_text_line_fast(const char* str, u32 x, u32 y, u32 color)
{
   if (!str || y >= fb_height) return;
    u32 curr_x = x;
    u32 stride = get_stride();
    u32* buf = get_active_buffer();
    while (*str && curr_x + CHAR_WIDTH < fb_width)
    {
        char c = *str++;
        if (c < 32 || c > 126) {
            curr_x += CHAR_WIDTH;
            continue;
        }
        const u8* glyph = font_8x16[(u8)c];
        if (!glyph) {
            curr_x += CHAR_WIDTH;
            continue;
        }
        for (u32 dy = 0; dy < CHAR_HEIGHT; dy++)
        {
            u8 row = glyph[dy];
            if (!row) continue;
            u32 screen_y = y + dy * font_scale;
            if (screen_y >= fb_height) break;
            u32* dest = &buf[screen_y * stride + curr_x];
            if (row & 0x80) dest[0] = color;
            if (row & 0x40) dest[1] = color;
            if (row & 0x20) dest[2] = color;
            if (row & 0x10) dest[3] = color;
            if (row & 0x08) dest[4] = color;
            if (row & 0x04) dest[5] = color;
            if (row & 0x02) dest[6] = color;
            if (row & 0x01) dest[7] = color;
        }
        curr_x += CHAR_WIDTH;
    }
}
void putpixels_horizontal(u32 x, u32 y, u32 width, u32 color)
{
    if (x >= fb_width || y >= fb_height || width == 0) return;
    if (x + width > fb_width) width = fb_width - x;
    u32 stride = get_stride();
    u32* buf = get_active_buffer();
    u32* dest = &buf[y * stride + x];
    u32 blocks = width / 8;
    u32 remainder = width % 8;
    while (blocks--) {
        dest[0] = color; dest[1] = color; dest[2] = color; dest[3] = color;
        dest[4] = color; dest[5] = color; dest[6] = color; dest[7] = color;
        dest += 8;
    }
    while (remainder--) *dest++ = color;
}
u32 get_fb_width(void) { return fb_width; }
u32 get_fb_height(void) { return fb_height; }
u32 get_fb_pitch(void) { return fb_pitch; }
u32* get_framebuffer(void) { return framebuffer; }
u32* get_backbuffer(void) { return backbuffer; }
void set_font_scale(u32 scale) {
    if (scale >= 1 && scale <= 4)
        font_scale = scale;
}
u32 get_font_scale(void) {
    return font_scale;
}