#ifndef VGA_GUI_H
#define VGA_GUI_H
#include <types.h>
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000
#define VGA_BLACK 0x0
#define VGA_BLUE 0x1
#define VGA_GREEN 0x2
#define VGA_CYAN 0x3
#define VGA_RED 0x4
#define VGA_MAGENTA 0x5
#define VGA_BROWN 0x6
#define VGA_LIGHT_GRAY 0x7
#define VGA_DARK_GRAY 0x8
#define VGA_LIGHT_BLUE 0x9
#define VGA_LIGHT_GREEN 0xA
#define VGA_LIGHT_CYAN 0xB
#define VGA_LIGHT_RED 0xC
#define VGA_LIGHT_MAGENTA 0xD
#define VGA_YELLOW 0xE
#define VGA_WHITE 0xF
#define VGA_ATTR(fg, bg) ((fg) | ((bg) << 4))
void vga_gui_init(void);
void vga_gui_clear(u8 color);
void vga_gui_put_char(int x, int y, char c, u8 attr);
void vga_gui_put_string(int x, int y, const char* str, u8 attr);
void vga_gui_draw_rect(int x, int y, int width, int height, u8 attr);
void vga_gui_draw_window(int x, int y, int width, int height, const char* title);
void vga_gui_draw_button(int x, int y, int width, int height, const char* text, int pressed);
void vga_gui_run(void);
#endif