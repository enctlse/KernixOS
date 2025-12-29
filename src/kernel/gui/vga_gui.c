#include "vga_gui.h"
static int vga_strlen(const char* str) {
    int len = 0;
    while (*str++) len++;
    return len;
}
void vga_set_text_mode(void) {
    vga_gui_clear(VGA_BLACK);
}
static volatile u16* vga_buffer = (volatile u16*)VGA_MEMORY;
static inline u16 vga_char(char c, u8 attr) {
    return (u16)c | ((u16)attr << 8);
}
void vga_gui_init(void) {
    vga_set_text_mode();
    vga_gui_clear(VGA_BLACK);
}
void vga_gui_clear(u8 color) {
    u16 clear_char = vga_char(' ', VGA_ATTR(VGA_WHITE, color));
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = clear_char;
    }
}
void vga_gui_put_char(int x, int y, char c, u8 attr) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        vga_buffer[y * VGA_WIDTH + x] = vga_char(c, attr);
    }
}
void vga_gui_put_string(int x, int y, const char* str, u8 attr) {
    while (*str && x < VGA_WIDTH) {
        vga_gui_put_char(x++, y, *str++, attr);
    }
}
void vga_gui_draw_rect(int x, int y, int width, int height, u8 attr) {
    for (int i = x; i < x + width && i < VGA_WIDTH; i++) {
        if (y >= 0 && y < VGA_HEIGHT) vga_gui_put_char(i, y, '-', attr);
        if (y + height - 1 >= 0 && y + height - 1 < VGA_HEIGHT) vga_gui_put_char(i, y + height - 1, '-', attr);
    }
    for (int i = y; i < y + height && i < VGA_HEIGHT; i++) {
        if (x >= 0 && x < VGA_WIDTH) vga_gui_put_char(x, i, '|', attr);
        if (x + width - 1 >= 0 && x + width - 1 < VGA_WIDTH) vga_gui_put_char(x + width - 1, i, '|', attr);
    }
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) vga_gui_put_char(x, y, '+', attr);
    if (x + width - 1 >= 0 && x + width - 1 < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) vga_gui_put_char(x + width - 1, y, '+', attr);
    if (x >= 0 && x < VGA_WIDTH && y + height - 1 >= 0 && y + height - 1 < VGA_HEIGHT) vga_gui_put_char(x, y + height - 1, '+', attr);
    if (x + width - 1 >= 0 && x + width - 1 < VGA_WIDTH && y + height - 1 >= 0 && y + height - 1 < VGA_HEIGHT) vga_gui_put_char(x + width - 1, y + height - 1, '+', attr);
}
void vga_gui_draw_window(int x, int y, int width, int height, const char* title) {
    vga_gui_draw_rect(x, y, width, height, VGA_ATTR(VGA_WHITE, VGA_BLUE));
    vga_gui_put_string(x + 2, y, title, VGA_ATTR(VGA_YELLOW, VGA_BLUE));
    for (int dy = 1; dy < height - 1; dy++) {
        for (int dx = 1; dx < width - 1; dx++) {
            vga_gui_put_char(x + dx, y + dy, ' ', VGA_ATTR(VGA_WHITE, VGA_BLACK));
        }
    }
}
void vga_gui_draw_button(int x, int y, int width, int height, const char* text, int pressed) {
    u8 attr = pressed ? VGA_ATTR(VGA_BLACK, VGA_WHITE) : VGA_ATTR(VGA_WHITE, VGA_BLUE);
    u8 shadow_attr = VGA_ATTR(VGA_BLACK, VGA_DARK_GRAY);
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            vga_gui_put_char(x + dx, y + dy, ' ', attr);
        }
    }
    int text_x = x + (width - vga_strlen(text)) / 2;
    int text_y = y + height / 2;
    vga_gui_put_string(text_x, text_y, text, attr);
    if (!pressed) {
        for (int dx = 1; dx < width + 1 && x + dx < VGA_WIDTH; dx++) {
            if (y + height < VGA_HEIGHT) vga_gui_put_char(x + dx, y + height, ' ', shadow_attr);
        }
        for (int dy = 1; dy < height + 1 && y + dy < VGA_HEIGHT; dy++) {
            if (x + width < VGA_WIDTH) vga_gui_put_char(x + width, y + dy, ' ', shadow_attr);
        }
    }
}
void vga_gui_run(void) {
    static int frame_count = 0;
    frame_count++;
    if (frame_count % 1000 == 0) {
        vga_gui_clear(VGA_BLUE);
    }
    vga_gui_draw_window(10, 5, 60, 15, "Terminal");
    vga_gui_draw_button(20, 10, 12, 3, "OK", 0);
    vga_gui_draw_button(35, 10, 12, 3, "Cancel", 0);
    char status[32];
    status[0] = 'F'; status[1] = 'r'; status[2] = 'a'; status[3] = 'm'; status[4] = 'e';
    status[5] = ':'; status[6] = ' '; status[7] = '0' + (frame_count / 1000) % 10;
    status[8] = '0' + (frame_count / 100) % 10; status[9] = '0' + (frame_count / 10) % 10;
    status[10] = '0' + frame_count % 10; status[11] = '\0';
    vga_gui_put_string(10, 2, status, VGA_ATTR(VGA_YELLOW, VGA_BLUE));
}