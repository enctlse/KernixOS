#include <gui/gui.h>
#include <string/string.h>
#include <kernel/display/visual.h>
#include <drivers/ps2/mouse/mouse.h>
#include <kernel/shell/functions.h>
#include <config/user.h>
#include <kernel/shell/functions.h>
#include <kernel/mem/kernel_memory/kernel_memory.h>
#include <fs/vfs/vfs.h>
typedef struct {
    void (*func)(const char*);
    const char* name;
} system_cmd_t;
gui_terminal_state_t* current_terminal_state = NULL;
extern char cwd[];
extern int console_mode;
void gui_cmd_exit(const char* args);
void gui_generate_prompt(char* buffer, size_t size) {
    (void)size;
    str_copy(buffer, User);
    str_append(buffer, "@");
    str_append(buffer, Host);
    str_append(buffer, ":");
    if (cwd && str_len(cwd) > 0) {
        if (str_len(cwd) > 1 && cwd[str_len(cwd) - 1] == '/') {
            char prompt_cwd[256];
            str_copy(prompt_cwd, cwd);
            prompt_cwd[str_len(cwd) - 1] = '\0';
            str_append(buffer, prompt_cwd);
        } else {
            str_append(buffer, cwd);
        }
    } else {
        str_append(buffer, "/");
    }
    str_append(buffer, "$ ");
}
void gui_terminal_update_window_size(gui_terminal_state_t* state, gui_window_t* window) {
    if (window->width > 10)
        state->window_width_chars = (window->width - 10) / CHAR_WIDTH;
    else
        state->window_width_chars = 1;
    int effective_height = window->height;
    int screen_bottom = get_fb_height();
    if (window->y + window->height > screen_bottom) {
        effective_height = screen_bottom - window->y;
    }
    if (effective_height < WINDOW_TITLE_HEIGHT + 10) effective_height = WINDOW_TITLE_HEIGHT + 10;
    if (effective_height > WINDOW_TITLE_HEIGHT + 5)
        state->window_height_lines = (effective_height - WINDOW_TITLE_HEIGHT - 10) / CHAR_HEIGHT;
    else
        state->window_height_lines = 1;
    state->scrollbar_visible = state->output_line_count > state->window_height_lines;
    if (state->scrollbar_visible) {
        state->scrollbar_width = 15;
        state->scrollbar_height = effective_height - WINDOW_TITLE_HEIGHT - 10;
        state->scrollbar_x = window->x + window->width - state->scrollbar_width - 5;
        state->scrollbar_y = window->y + WINDOW_TITLE_HEIGHT + 5;
        int total_lines = state->output_line_count;
        int visible_lines = state->window_height_lines;
        if (total_lines > 0) {
            state->scrollbar_thumb_height = (state->scrollbar_height * visible_lines) / total_lines;
            if (state->scrollbar_thumb_height < 20) state->scrollbar_thumb_height = 20;
            if (state->scrollbar_thumb_height > state->scrollbar_height)
                state->scrollbar_thumb_height = state->scrollbar_height;
            int scrollable_lines = total_lines - visible_lines;
            if (scrollable_lines > 0) {
                state->scrollbar_thumb_y = state->scrollbar_y +
                    ((state->scrollbar_height - state->scrollbar_thumb_height) *
                      state->scroll_offset) / scrollable_lines;
            } else {
                state->scrollbar_thumb_y = state->scrollbar_y;
            }
        }
    }
}
void gui_terminal_add_line(gui_terminal_state_t* state, const char* line, u32 color) {
    if (!line) return;
    int max_lines = GUI_OUTPUT_LINES;
    if (state->output_line_count >= max_lines) {
        for (int i = 0; i < max_lines - 1; i++) {
            str_copy(state->output_buffer[i], state->output_buffer[i+1]);
            state->output_colors[i] = state->output_colors[i+1];
        }
        state->output_line_count = max_lines - 1;
    }
    int copy_len = str_len(line);
    if (copy_len >= GUI_OUTPUT_COLS) copy_len = GUI_OUTPUT_COLS - 1;
    for (int i = 0; i < copy_len; i++) {
        state->output_buffer[state->output_line_count][i] = line[i];
    }
    state->output_buffer[state->output_line_count][copy_len] = '\0';
    state->output_colors[state->output_line_count] = color;
    state->output_line_count++;
    state->scroll_offset = 0;
}
void gui_terminal_print(gui_terminal_state_t* state, const char *text, u32 color) {
    if (!text) return;
    static char current_line[MAX_LINE_LENGTH];
    int current_pos = 0;
    while (*text) {
        if (*text == '\n') {
            current_line[current_pos] = '\0';
            int pos = 0;
            while (pos < current_pos) {
                int len = current_pos - pos;
                if (len > state->window_width_chars) len = state->window_width_chars;
                char line[MAX_LINE_LENGTH];
                for (int i = 0; i < len; i++) {
                    line[i] = current_line[pos + i];
                }
                line[len] = '\0';
                gui_terminal_add_line(state, line, color);
                pos += len;
            }
            current_pos = 0;
            text++;
        } else {
            current_line[current_pos++] = *text++;
        }
    }
    if (current_pos > 0) {
        current_line[current_pos] = '\0';
        int pos = 0;
        while (pos < current_pos) {
            int len = current_pos - pos;
            if (len > state->window_width_chars) len = state->window_width_chars;
            char line[MAX_LINE_LENGTH];
            for (int i = 0; i < len; i++) {
                line[i] = current_line[pos + i];
            }
            line[len] = '\0';
            gui_terminal_add_line(state, line, color);
            pos += len;
        }
    }
}
void gui_terminal_clear(gui_terminal_state_t* state) {
    state->output_line_count = 0;
    state->scroll_offset = 0;
    state->cursor_blink_timer = 0;
    char prompt[64];
    gui_generate_prompt(prompt, sizeof(prompt));
    gui_terminal_add_line(state, prompt, TERM_COLOR_PROMPT);
    state->input_pos = str_len(prompt);
}
void gui_terminal_init(gui_terminal_state_t* state) {
    state->output_line_count = 0;
    state->cursor_visible = 1;
    state->show_prompt = 1;
    state->window_width_chars = 80;
    state->window_height_lines = 25;
    state->scroll_offset = 0;
    state->cursor_blink_timer = 0;
    char prompt[64];
    gui_generate_prompt(prompt, sizeof(prompt));
    gui_terminal_add_line(state, prompt, TERM_COLOR_PROMPT);
    state->input_pos = str_len(prompt);
}
static system_cmd_t system_commands[] = {
    {cmd_echo, "echo"},
    {cmd_clear, "clear"},
    {cmd_help, "help"},
    {cmd_modules, "modules"},
    {cmd_sysinfo, "fetch"},
    {cmd_cpuinfo, "cpuinfo"},
    {cmd_cal, "calendar"},
    {cmd_date, "date"},
    {cmd_uptime, "uptime"},
    {cmd_time, "time"},
    {cmd_cat, "cat"},
    {cmd_ls, "ls"},
    {cmd_cd, "cd"},
    {cmd_mkdir, "mkdir"},
    {cmd_touch, "touch"},
    {cmd_poweroff, "poweroff"},
    {cmd_reboot, "reboot"},
    {cmd_shutdown, "shutdown"},
    {cmd_gui, "gui"},
    {cmd_cls, "cls"},
    {gui_cmd_exit, "exit"},
    {NULL, NULL}
};
void gui_cmd_exit(const char* args) {
    (void)args;
    gui_running = 0;
    extern void graphics_disable_double_buffering(void);
    extern void console_window_init(void);
    extern void cursor_draw(void);
    extern void mouse_set_callback(void (*cb)(int32_t, int32_t, uint8_t));
    extern void usb_mouse_set_callback(void (*cb)(int32_t, int32_t, uint8_t));
    extern void usb_keyboard_set_callback(void (*cb)(int));
    extern void console_handle_key(int);
    graphics_disable_double_buffering();
    console_window_init();
    cursor_draw();
    mouse_set_callback(NULL);
    usb_mouse_set_callback(NULL);
    usb_keyboard_set_callback(console_handle_key);
}
system_cmd_t* gui_find_system_cmd(const char *name) {
    for (int i = 0; system_commands[i].name != NULL; i++) {
        const char *a = name;
        const char *b = system_commands[i].name;
        int match = 1;
        while (*a && *b) {
            if (*a != *b) {
                match = 0;
                break;
            }
            a++;
            b++;
        }
        if (match && *a == '\0' && *b == '\0') {
            return &system_commands[i];
        }
    }
    return NULL;
}
void gui_execute(gui_terminal_state_t* state, const char *input) {
    while (*input == ' ') input++;
    if (*input == '\0') return;
    const char *end = input;
    while (*end && *end != ' ') end++;
    char cmd_name[64];
    int len = end - input;
    if (len < 0) len = 0;
    if (len >= 64) len = 63;
    for (int i = 0; i < len; i++) {
        cmd_name[i] = input[i];
    }
    cmd_name[len] = '\0';
    const char *args = end;
    while (*args == ' ') args++;
    system_cmd_t *sys_cmd = gui_find_system_cmd(cmd_name);
    if (sys_cmd) {
        current_terminal_state = state;
        sys_cmd->func(args);
        current_terminal_state = NULL;
    } else {
        gui_terminal_print(state, cmd_name, TERM_COLOR_ERROR);
        gui_terminal_print(state, ": command not found\n", TERM_COLOR_ERROR);
    }
}
void terminal_print(const char *text, u32 color) {
    if (current_terminal_state) {
        gui_terminal_print(current_terminal_state, text, color);
    } else {
        extern void string(const char *str, u32 color);
        int saved_gui_mode = 0;
        extern int gui_mode;
        if (gui_mode) {
            saved_gui_mode = 1;
            gui_mode = 0;
        }
        string(text, color);
        if (saved_gui_mode) {
            gui_mode = 1;
        }
    }
}
void cmd_gui(const char* args) {
    (void)args;
    extern int gui_running;
    extern void gui_init(void);
    if (gui_running) {
        terminal_print("GUI is already running!\n", TERM_COLOR_ERROR);
        return;
    }
    gui_init();
}
void gui_terminal_handle_key(gui_terminal_state_t* state, char key) {
    state->scroll_offset = 0;
    char prompt[64];
    gui_generate_prompt(prompt, sizeof(prompt));
    int prompt_len = str_len(prompt);
    if (key == '\b') {
        if (state->input_pos > prompt_len) {
            state->input_pos--;
            state->output_buffer[state->output_line_count - 1][state->input_pos] = '\0';
        }
    } else if (key == '\n') {
        char* command = state->output_buffer[state->output_line_count - 1] + prompt_len;
        if (str_len(command) > 0) {
            gui_execute(state, command);
        }
        char new_prompt[64];
        gui_generate_prompt(new_prompt, sizeof(new_prompt));
        gui_terminal_add_line(state, new_prompt, TERM_COLOR_PROMPT);
        state->input_pos = str_len(new_prompt);
        state->scroll_offset = 0;
    } else if (key >= 32 && key <= 126) {
        state->output_buffer[state->output_line_count - 1][state->input_pos++] = key;
        state->output_buffer[state->output_line_count - 1][state->input_pos] = '\0';
    }
}
void gui_terminal_handle_scroll(gui_terminal_state_t* state, int delta) {
    int max_scroll = state->output_line_count - state->window_height_lines;
    if (max_scroll < 0) max_scroll = 0;
    if (delta > 0) {
          state->scroll_offset += 3;
          if (state->scroll_offset > max_scroll) state->scroll_offset = max_scroll;
    } else {
          state->scroll_offset -= 3;
          if (state->scroll_offset < 0) state->scroll_offset = 0;
    }
}
void gui_terminal_handle_scrollbar_click(gui_terminal_state_t* state, int x, int y) {
    if (!state->scrollbar_visible) return;
    if (x >= state->scrollbar_x && x < state->scrollbar_x + state->scrollbar_width &&
        y >= state->scrollbar_thumb_y && y < state->scrollbar_thumb_y + state->scrollbar_thumb_height) {
        state->scrollbar_dragging = 1;
        state->scrollbar_drag_start_y = y - state->scrollbar_thumb_y;
    } else if (x >= state->scrollbar_x && x < state->scrollbar_x + state->scrollbar_width &&
               y >= state->scrollbar_y && y < state->scrollbar_y + state->scrollbar_height) {
        int total_lines = state->output_line_count;
        int visible_lines = state->window_height_lines;
        int scrollable_lines = total_lines - visible_lines;
        if (scrollable_lines > 0) {
            int click_pos = y - state->scrollbar_y;
            int track_height = state->scrollbar_height;
            state->scroll_offset = (click_pos * scrollable_lines) / track_height;
            if (state->scroll_offset < 0) state->scroll_offset = 0;
            if (state->scroll_offset > scrollable_lines) state->scroll_offset = scrollable_lines;
        }
    }
}
void gui_terminal_handle_scrollbar_drag(gui_terminal_state_t* state, int x, int y) {
    (void)x;
    if (!state->scrollbar_dragging || !state->scrollbar_visible) return;
    int total_lines = state->output_line_count;
    int visible_lines = state->window_height_lines;
    int scrollable_lines = total_lines - visible_lines;
    if (scrollable_lines <= 0) return;
    int track_height = state->scrollbar_height - state->scrollbar_thumb_height;
    int drag_pos = y - state->scrollbar_drag_start_y - state->scrollbar_y;
    if (drag_pos < 0) drag_pos = 0;
    if (drag_pos > track_height) drag_pos = track_height;
    state->scroll_offset = (drag_pos * scrollable_lines) / track_height;
    if (state->scroll_offset < 0) state->scroll_offset = 0;
    if (state->scroll_offset > scrollable_lines) state->scroll_offset = scrollable_lines;
}
void gui_terminal_handle_scrollbar_release(gui_terminal_state_t* state) {
    state->scrollbar_dragging = 0;
}
void gui_terminal_draw(gui_terminal_state_t* state, gui_window_t* window, int text_x, int text_y) {
    gui_terminal_update_window_size(state, window);
    gui_set_needs_redraw(1);
    state->cursor_blink_timer++;
    if (state->cursor_blink_timer % 30 == 0) {
        state->cursor_visible = !state->cursor_visible;
    }
    int char_h = fm_get_char_height();
    int visible_lines = state->window_height_lines;
    int start_idx = 0;
    if (state->output_line_count > visible_lines) {
        start_idx = state->output_line_count - visible_lines - state->scroll_offset;
        if (start_idx < 0) start_idx = 0;
    }
    int window_bottom = window->y + window->height;
    int screen_bottom = get_fb_height();
    int effective_window_bottom = (window_bottom > screen_bottom) ? screen_bottom : window_bottom;
    int effective_height = effective_window_bottom - window->y;
    int available_height = effective_height - WINDOW_TITLE_HEIGHT - 10;
    if (available_height < 0) available_height = 0;
    int actual_visible_lines = available_height / char_h;
    if (actual_visible_lines < 1) actual_visible_lines = 1;
    if (state->scrollbar_visible && state->scrollbar_y < effective_window_bottom) {
        int scrollbar_bottom = state->scrollbar_y + state->scrollbar_height;
        int visible_scrollbar_height = (scrollbar_bottom > effective_window_bottom) ?
            effective_window_bottom - state->scrollbar_y : state->scrollbar_height;
        if (visible_scrollbar_height > 0) {
            draw_rect(state->scrollbar_x, state->scrollbar_y,
                      state->scrollbar_width, visible_scrollbar_height, 0xFF2C2C2C);
            if (state->scrollbar_thumb_y < effective_window_bottom &&
                state->scrollbar_thumb_y + state->scrollbar_thumb_height > state->scrollbar_y) {
                int visible_thumb_y = state->scrollbar_thumb_y;
                int visible_thumb_height = state->scrollbar_thumb_height;
                if (visible_thumb_y < state->scrollbar_y) {
                    visible_thumb_height -= (state->scrollbar_y - visible_thumb_y);
                    visible_thumb_y = state->scrollbar_y;
                }
                if (visible_thumb_y + visible_thumb_height > effective_window_bottom) {
                    visible_thumb_height = effective_window_bottom - visible_thumb_y;
                }
                if (visible_thumb_height > 0) {
                    u32 thumb_color = state->scrollbar_dragging ? 0xFF666666 : 0xFF888888;
                    draw_rect(state->scrollbar_x + 2, visible_thumb_y,
                              state->scrollbar_width - 4, visible_thumb_height, thumb_color);
                }
            }
        }
    }
    int cur_y = text_y;
    int line_idx = 0;
    int max_output_lines = actual_visible_lines;
    for (int i = start_idx; i < state->output_line_count && line_idx < max_output_lines; i++) {
        if (cur_y + char_h > effective_window_bottom) break;
        char* line = state->output_buffer[i];
        u32 color = state->output_colors[i];
        int line_len = str_len(line);
        int display_chars = state->scrollbar_visible ?
            state->window_width_chars - 2 : state->window_width_chars;
        if (i == state->output_line_count - 1) {
            char prompt[64];
            gui_generate_prompt(prompt, sizeof(prompt));
            int prompt_len = str_len(prompt);
            int input_len = line_len - prompt_len;
            int input_pos = 0;
            int y_offset = 0;
            while ((y_offset == 0 || input_pos < input_len) && line_idx < max_output_lines) {
                if (cur_y + char_h > effective_window_bottom) break;
                int available_chars = display_chars;
                int start_x = text_x;
                if (y_offset == 0) {
                    if (prompt_len > 0) {
                        int prompt_display = (prompt_len > display_chars) ? display_chars : prompt_len;
                        char prompt_part[prompt_display + 1];
                        for (int j = 0; j < prompt_display; j++) {
                            prompt_part[j] = line[j];
                        }
                        prompt_part[prompt_display] = '\0';
                        gui_draw_text_line_fast(prompt_part, text_x, cur_y, TERM_COLOR_PROMPT);
                        available_chars -= prompt_display;
                        start_x += prompt_display * CHAR_WIDTH;
                    }
                }
                int len = input_len - input_pos;
                if (len > available_chars) len = available_chars;
                if (len > 0) {
                    char input_part[len + 1];
                    for (int j = 0; j < len; j++) {
                        input_part[j] = line[prompt_len + input_pos + j];
                    }
                    input_part[len] = '\0';
                    gui_draw_text_line_fast(input_part, start_x, cur_y, TERM_COLOR_INPUT);
                    input_pos += len;
                }
                cur_y += char_h;
                line_idx++;
                y_offset++;
            }
        } else {
           int pos = 0;
while (pos < line_len && line_idx < max_output_lines) {
    int len = line_len - pos;
    if (len > display_chars) len = display_chars;
    char part[display_chars + 1];
    for (int j = 0; j < len; j++) {
        part[j] = line[pos + j];
    }
    part[len] = '\0';
    gui_draw_text_line_fast(part, text_x, cur_y, color);
    cur_y += char_h;
    line_idx++;
    pos += len;
    if (cur_y + char_h > effective_window_bottom) break;
}
    }
}
}