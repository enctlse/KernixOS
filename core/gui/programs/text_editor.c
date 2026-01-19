#include <gui/gui.h>
#include <string/string.h>
#include <kernel/display/visual.h>
#include <kernel/display/fonts/typeface.h>

gui_text_editor_state_t* text_editor_states = NULL;
static int text_editor_state_used[MAX_WINDOWS] = {0};

void gui_text_editor_init(gui_text_editor_state_t* state) {
    state->line_count = 1;
    state->cursor_line = 0;
    state->cursor_col = 0;
    state->scroll_offset = 0;
    str_copy(state->lines[0], "");
}

void gui_draw_text_editor(gui_window_t* window) {
    if (!window || !str_equals(window->title, "Text Editor") || !window->text_editor_state) return;
    gui_text_editor_state_t* state = window->text_editor_state;
    int mx = window->x;
    int my = window->y;
    int mw = window->width;
    int mh = window->height;
    int bg_x = mx;
    int bg_y = my + 25;
    int bg_w = mw;
    int bg_h = mh - 25;
    if (bg_x >= 0 && bg_y >= 0 && bg_w > 0 && bg_h > 0) {
        draw_rect(bg_x, bg_y, bg_w, bg_h, 0xFFFFFFFF);
    }
    int text_x = bg_x + 50; // space for line numbers
    int text_y = bg_y + 10;
    int line_height = 16;
    int visible_lines = (bg_h - 20) / line_height;
    int start_line = state->scroll_offset;
    int end_line = start_line + visible_lines;
    if (end_line > state->line_count) end_line = state->line_count;
    for (int i = start_line; i < end_line; i++) {
        // draw line number
        char line_num[10];
        str_copy(line_num, "");
        str_append_uint(line_num, i + 1);
        str_append(line_num, ": ");
        gui_draw_text_line_fast(line_num, (u32)(bg_x + 10), (u32)text_y, 0xFF888888);
        // draw text
        gui_draw_text_line_fast(state->lines[i], (u32)text_x, (u32)text_y, 0xFF000000);
        text_y += line_height;
    }
    // draw cursor
    int cursor_x = text_x + state->cursor_col * 8;
    int cursor_y = bg_y + 10 + (state->cursor_line - start_line) * line_height;
    if (cursor_y >= bg_y + 10 && cursor_y < bg_y + bg_h - 10) {
        draw_rect(cursor_x, cursor_y, 2, line_height, 0xFF000000);
    }
}

void gui_text_editor_handle_key(gui_text_editor_state_t* state, int key) {
    if (key == '\b') {
        if (state->cursor_col > 0) {
            state->cursor_col--;
            // shift left
            for (int i = state->cursor_col; i < GUI_TEXT_EDITOR_COLS - 1; i++) {
                state->lines[state->cursor_line][i] = state->lines[state->cursor_line][i + 1];
            }
        } else if (state->cursor_line > 0) {
            // merge with previous line
            int prev_len = str_len(state->lines[state->cursor_line - 1]);
            str_append(state->lines[state->cursor_line - 1], state->lines[state->cursor_line]);
            state->cursor_line--;
            state->cursor_col = prev_len;
            // shift lines up
            for (int i = state->cursor_line + 1; i < state->line_count - 1; i++) {
                str_copy(state->lines[i], state->lines[i + 1]);
            }
            state->line_count--;
        }
    } else if (key == '\n') {
        // insert new line
        if (state->line_count < GUI_TEXT_EDITOR_LINES - 1) {
            for (int i = state->line_count; i > state->cursor_line + 1; i--) {
                str_copy(state->lines[i], state->lines[i - 1]);
            }
            str_copy(state->lines[state->cursor_line + 1], state->lines[state->cursor_line] + state->cursor_col);
            state->lines[state->cursor_line][state->cursor_col] = '\0';
            state->cursor_line++;
            state->cursor_col = 0;
            state->line_count++;
        }
    } else if (key >= 32 && key <= 126) {
        if (state->cursor_col < GUI_TEXT_EDITOR_COLS - 1) {
            // shift right
            for (int i = GUI_TEXT_EDITOR_COLS - 1; i > state->cursor_col; i--) {
                state->lines[state->cursor_line][i] = state->lines[state->cursor_line][i - 1];
            }
            state->lines[state->cursor_line][state->cursor_col] = key;
            state->cursor_col++;
        }
    }
    // adjust scroll
    if (state->cursor_line < state->scroll_offset) {
        state->scroll_offset = state->cursor_line;
    } else if (state->cursor_line >= state->scroll_offset + 10) { // assume visible 10 lines
        state->scroll_offset = state->cursor_line - 9;
    }
}