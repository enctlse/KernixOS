#pragma once
#include <gui/gui.h>
void gui_terminal_init();
void gui_terminal_clear();
void gui_terminal_print(const char *text, u32 color);
void gui_terminal_draw(gui_window_t* window, int text_x, int text_y);
void gui_terminal_handle_key(char key);