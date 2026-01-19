#include "print.h"
 #include <kernel/display/visual.h>
#include <kernel/communication/serial.h>
#include <ui/fonts/font_8x16.h>
#include <kernel/shell/functions.h>

static void putchar_bootstrap_at(char c, u32 x, u32 y, u32 color)
{
    const u8 *glyph = font_8x16[(u8)c];
    for (int glyph_y = 0; glyph_y < 8; glyph_y++)
    {
        u8 row = glyph[glyph_y];
        for (int glyph_x = 0; glyph_x < 8; glyph_x++)
        {
            if (row & (1 << (7 - glyph_x)))
            {
                for (u32 scale_y = 0; scale_y < font_scale; scale_y++) {
                    for (u32 scale_x = 0; scale_x < font_scale; scale_x++) {
                        putpixel(x + glyph_x * font_scale + scale_x, y + glyph_y * font_scale + scale_y, color);
                    }
                }
            }
        }
    }
}

void putchar_bootstrap(char c, u32 color)
{
    u32 char_width = 8 * font_scale;
    u32 char_height = 8 * font_scale;
    u32 char_spacing = char_width;
    u32 line_height = char_height + 2 * font_scale;
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y += line_height;
        return;
    }
    if (cursor_x + char_width >= fb_width)
    {
        cursor_x = 0;
        cursor_y += line_height;
    }
    putchar_bootstrap_at(c, cursor_x, cursor_y, color);
    cursor_x += char_spacing;
}

void printbs(const char *str, u32 color)
{
    for (size_t char_index = 0; str[char_index]; char_index++)
    {
        putchar_bootstrap(str[char_index], color);
    }
    printf("%s", str);
}