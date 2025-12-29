#ifndef CONSOLE_DOS_H
#define CONSOLE_DOS_H
#include <types.h>
#define CONSOLE_PADDING_X 0
void console_window_init(void);
void console_window_clear(u32 color);
u32 console_window_get_start_y(void);
u32 console_window_get_max_y(void);
void console_window_check_scroll(void);
void console_window_update_layout(void);
void console_window_scroll_lines(u32 lines);
u32 console_window_get_visible_lines(void);
int console_window_needs_scroll(void);
#endif