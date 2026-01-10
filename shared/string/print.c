#include "print.h"
#include <kernel/graph/graphics.h>
#include <kernel/graph/fm.h>
#include <kernel/console/functions.h>
#include <gui/programs/terminal.h>
extern int gui_mode;
static inline void putchar_fast(char c, u32 x, u32 y, u32 color)
{
    if (c < 32 || c > 126) return;
    const u8 *glyph = fm_get_glyph((u8)c);
    if (!glyph) return;
    u32* buf = graphics_is_double_buffering_enabled() ? get_backbuffer() : get_framebuffer();
    if (!buf) return;
    u32 stride = get_fb_pitch() / 4;
    u32 glyph_height = fm_get_char_height();
    if (font_scale == 1) {
        for (u32 dy = 0; dy < glyph_height; dy++) {
            u8 row = glyph[dy];
            if (!row) continue;
            u32* dest = &buf[(y + dy) * stride + x];
            if (row & 0x80) dest[0] = color;
            if (row & 0x40) dest[1] = color;
            if (row & 0x20) dest[2] = color;
            if (row & 0x10) dest[3] = color;
            if (row & 0x08) dest[4] = color;
            if (row & 0x04) dest[5] = color;
            if (row & 0x02) dest[6] = color;
            if (row & 0x01) dest[7] = color;
        }
    } else {
        u32 glyph_width = fm_get_char_width();
        for (u32 dy = 0; dy < glyph_height; dy++) {
            u8 row = glyph[dy];
            if (!row) continue;
            for (u32 sy = 0; sy < font_scale; sy++) {
                u32 screen_y = y + dy * font_scale + sy;
                if (screen_y >= get_fb_height()) break;
                u32* dest = &buf[screen_y * stride + x];
                for (u32 dx = 0; dx < glyph_width; dx++) {
                    if (row & (0x80 >> dx)) {
                        for (u32 sx = 0; sx < font_scale; sx++) {
                            u32 px = dx * font_scale + sx;
                            if (x + px < get_fb_width()) {
                                dest[px] = color;
                            }
                        }
                    }
                }
            }
        }
    }
}
void putchar(char c, u32 color)
{
    u32 char_width = fm_get_char_width() * font_scale;
    u32 char_height = fm_get_char_height() * font_scale;
    u32 fb_w = get_fb_width();
    u32 fb_h = get_fb_height();
    u32 banner_h = console_window_get_start_y();
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += char_height;
        if (cursor_y + char_height > fb_h) {
            scroll_up(1);
            cursor_y -= char_height;
            if (cursor_y < banner_h) cursor_y = banner_h;
        }
        return;
    }
    if (c == '\r') {
        cursor_x = 0;
        return;
    }
    if (c == '\t') {
        for (int i = 0; i < 4; i++) {
            if (cursor_x + char_width >= fb_w) {
                cursor_x = 0;
                cursor_y += char_height;
                if (cursor_y + char_height > fb_h) {
                    scroll_up(1);
                    cursor_y -= char_height;
                    if (cursor_y < banner_h) cursor_y = banner_h;
                }
            }
            cursor_x += char_width;
        }
        return;
    }
    if (c == '\b') {
        if (cursor_x >= char_width) {
            cursor_x -= char_width;
            draw_rect(cursor_x, cursor_y, char_width, char_height, 0x00000000);
        }
        return;
    }
    if (cursor_x + char_width > fb_w) {
        cursor_x = 0;
        cursor_y += char_height;
        if (cursor_y + char_height > fb_h) {
            scroll_up(1);
            cursor_y -= char_height;
            if (cursor_y < banner_h) cursor_y = banner_h;
        }
    }
    if (c >= 32 && c <= 126) {
        putchar_fast(c, cursor_x, cursor_y, color);
    }
    cursor_x += char_width;
}
static char gui_buffer[1024] = {0};
static int gui_buffer_pos = 0;
static u32 gui_buffer_color = 0xFFFFFFFF;

void string(const char *str, u32 color)
{
    if (!str) return;
    if (gui_mode) {
        // In GUI mode, buffer output until newline
        while (*str) {
            if (*str == '\n' || gui_buffer_pos >= sizeof(gui_buffer) - 1) {
                gui_buffer[gui_buffer_pos] = '\0';
                if (gui_buffer_pos > 0) {
                    gui_terminal_print(gui_buffer, gui_buffer_color);
                    gui_buffer_pos = 0;
                }
                if (*str == '\n') {
                    gui_terminal_print("\n", gui_buffer_color);
                }
            } else {
                gui_buffer[gui_buffer_pos++] = *str;
                gui_buffer_color = color;  // Update color for current buffer
            }
            str++;
        }
        return;
    }
    while (*str) {
        putchar(*str, color);
        str++;
    }
    if (graphics_is_double_buffering_enabled()) {
        graphics_swap_buffers();
    }
}
void print(const char *str, u32 color)
{
    string(str, color);
}
void println(const char *str, u32 color)
{
    string(str, color);
    putchar('\n', color);
    if (graphics_is_double_buffering_enabled()) {
        graphics_swap_buffers();
    }
}
void IntToString(int value, char *buffer)
{
    char temp[12];
    int i = 0;
    int isNegative = 0;
    if (value < 0) {
        isNegative = 1;
        value = -value;
    }
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    while (value > 0) {
        temp[i++] = (value % 10) + '0';
        value /= 10;
    }
    int j = 0;
    if (isNegative) {
        buffer[j++] = '-';
    }
    while (i > 0) {
        buffer[j++] = temp[--i];
    }
    buffer[j] = '\0';
}
void printInt(int value, u32 color)
{
    char buffer[12];
    IntToString(value, buffer);
    string(buffer, color);
}
void print_uint(u32 num, u32 color)
{
    if (num == 0) {
        putchar('0', color);
        return;
    }
    char buffer[12];
    int i = 0;
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i > 0) {
        putchar(buffer[--i], color);
    }
}
void print_hex(u32 num, u32 color)
{
    const char hex[] = "0123456789ABCDEF";
    char buffer[10];
    buffer[0] = '0';
    buffer[1] = 'x';
    for (int i = 0; i < 8; i++) {
        buffer[2 + i] = hex[(num >> ((7 - i) * 4)) & 0xF];
    }
    buffer[10] = '\0';
    string(buffer, color);
}
void print_hex64(u64 num, u32 color)
{
    const char hex[] = "0123456789ABCDEF";
    char buffer[18];
    buffer[0] = '0';
    buffer[1] = 'x';
    for (int i = 0; i < 16; i++) {
        buffer[2 + i] = hex[(num >> ((15 - i) * 4)) & 0xF];
    }
    buffer[18] = '\0';
    string(buffer, color);
}
void print_colored(const char *str, u32 fg_color, u32 bg_color)
{
    if (!str) return;
    u32 char_width = fm_get_char_width() * font_scale;
    u32 char_height = fm_get_char_height() * font_scale;
    while (*str) {
        if (*str == '\n') {
            putchar('\n', fg_color);
            str++;
            continue;
        }
        if (bg_color != 0) {
            draw_rect(cursor_x, cursor_y, char_width, char_height, bg_color);
        }
        putchar(*str, fg_color);
        str++;
    }
    if (graphics_is_double_buffering_enabled()) {
        graphics_swap_buffers();
    }
}
void set_cursor(u32 x, u32 y)
{
    cursor_x = x;
    cursor_y = y;
}
void get_cursor(u32 *x, u32 *y)
{
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}
void reset_cursor(void)
{
    cursor_x = 0;
    cursor_y = console_window_get_start_y();
}
void printf_simple(const char *fmt, u32 color, ...)
{
    print(fmt, color);
}