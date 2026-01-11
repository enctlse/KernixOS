#include <gui/gui.h>
#include <string/string.h>
#include <kernel/graph/graphics.h>
#include <drivers/ps2/mouse/mouse.h>
#include <kernel/console/functions.h>
#include <config/user.h>
#include <kernel/console/functions.h>
#define GUI_OUTPUT_LINES 200
#define GUI_OUTPUT_COLS 160
#define MAX_LINE_LENGTH 512
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 16
#define TERM_COLOR_DEFAULT  0xFFFFFFFF
#define TERM_COLOR_PROMPT   0xFF00FF00
#define TERM_COLOR_ERROR    0xFFFF0000
#define TERM_COLOR_COMMAND  0xFFFFFF00
#define TERM_COLOR_SUCCESS  0xFF00FF00
#define TERM_COLOR_INFO     0xFF00FFFF
typedef struct {
    void (*func)(const char*);
    const char* name;
} system_cmd_t;
typedef struct {
    char input_buffer[512];
    int input_pos;
    char output_buffer[GUI_OUTPUT_LINES][GUI_OUTPUT_COLS];
    u32 output_colors[GUI_OUTPUT_LINES];
    int output_line_count;
    int cursor_visible;
    int show_prompt;
    int window_width_chars;
    int window_height_lines;
    int scroll_offset;
    int scrollbar_visible;
    int scrollbar_x, scrollbar_y, scrollbar_width, scrollbar_height;
    int scrollbar_thumb_y, scrollbar_thumb_height;
    int scrollbar_dragging;
    int scrollbar_drag_start_y;
} gui_terminal_state_t;
static gui_terminal_state_t terminal_state = {0};
extern char cwd[];
extern int console_mode;
void gui_cmd_exit(const char* args);
void gui_generate_prompt(char* buffer, size_t size) {
    (void)size;
    str_copy(buffer, User);
    str_append(buffer, "@");
    str_append(buffer, Host);
    str_append(buffer, ":");
    if (str_len(cwd) > 1 && cwd[str_len(cwd) - 1] == '/') {
        char prompt_cwd[256];
        str_copy(prompt_cwd, cwd);
        prompt_cwd[str_len(cwd) - 1] = '\0';
        str_append(buffer, prompt_cwd);
    } else {
        str_append(buffer, cwd);
    }
    str_append(buffer, "$ ");
}
void gui_terminal_update_window_size(gui_window_t* window) {
    if (window->width > 10)
        terminal_state.window_width_chars = (window->width - 10) / CHAR_WIDTH;
    else
        terminal_state.window_width_chars = 1;
    int effective_height = window->height;
    int screen_bottom = get_fb_height();
    if (window->y + window->height > screen_bottom) {
        effective_height = screen_bottom - window->y;
    }
    if (effective_height < WINDOW_TITLE_HEIGHT + 10) effective_height = WINDOW_TITLE_HEIGHT + 10;
    if (effective_height > WINDOW_TITLE_HEIGHT + 5)
        terminal_state.window_height_lines = (effective_height - WINDOW_TITLE_HEIGHT - 10) / CHAR_HEIGHT;
    else
        terminal_state.window_height_lines = 1;
    terminal_state.scrollbar_visible = terminal_state.output_line_count > terminal_state.window_height_lines;
    if (terminal_state.scrollbar_visible) {
        terminal_state.scrollbar_width = 15;
        terminal_state.scrollbar_height = effective_height - WINDOW_TITLE_HEIGHT - 10;
        terminal_state.scrollbar_x = window->x + window->width - terminal_state.scrollbar_width - 5;
        terminal_state.scrollbar_y = window->y + WINDOW_TITLE_HEIGHT + 5;
        int total_lines = terminal_state.output_line_count;
        int visible_lines = terminal_state.window_height_lines;
        if (total_lines > 0) {
            terminal_state.scrollbar_thumb_height = (terminal_state.scrollbar_height * visible_lines) / total_lines;
            if (terminal_state.scrollbar_thumb_height < 20) terminal_state.scrollbar_thumb_height = 20;
            if (terminal_state.scrollbar_thumb_height > terminal_state.scrollbar_height)
                terminal_state.scrollbar_thumb_height = terminal_state.scrollbar_height;
            int scrollable_lines = total_lines - visible_lines;
            if (scrollable_lines > 0) {
                terminal_state.scrollbar_thumb_y = terminal_state.scrollbar_y +
                    ((terminal_state.scrollbar_height - terminal_state.scrollbar_thumb_height) *
                      terminal_state.scroll_offset) / scrollable_lines;
            } else {
                terminal_state.scrollbar_thumb_y = terminal_state.scrollbar_y;
            }
        }
    }
}
void gui_terminal_add_line(const char* line, u32 color) {
    if (!line) return;
    int max_lines = GUI_OUTPUT_LINES;
    if (terminal_state.output_line_count >= max_lines) {
        for (int i = 0; i < max_lines - 1; i++) {
            str_copy(terminal_state.output_buffer[i], terminal_state.output_buffer[i+1]);
            terminal_state.output_colors[i] = terminal_state.output_colors[i+1];
        }
        terminal_state.output_line_count = max_lines - 1;
    }
    int copy_len = str_len(line);
    if (copy_len >= GUI_OUTPUT_COLS) copy_len = GUI_OUTPUT_COLS - 1;
    str_copy(terminal_state.output_buffer[terminal_state.output_line_count], line);
    terminal_state.output_colors[terminal_state.output_line_count] = color;
    terminal_state.output_line_count++;
    terminal_state.scroll_offset = 0;
}
void gui_terminal_print(const char *text, u32 color) {
    if (!text) return;
    static char current_line[MAX_LINE_LENGTH];
    int current_pos = 0;
    while (*text) {
        if (*text == '\n') {
            current_line[current_pos] = '\0';
            gui_terminal_add_line(current_line, color);
            current_pos = 0;
            text++;
        } else {
            current_line[current_pos++] = *text++;
        }
    }
    if (current_pos > 0) {
        current_line[current_pos] = '\0';
        gui_terminal_add_line(current_line, color);
    }
}
void gui_terminal_clear() {
    terminal_state.output_line_count = 0;
    terminal_state.input_pos = 0;
    terminal_state.input_buffer[0] = '\0';
    terminal_state.scroll_offset = 0;
}
void gui_terminal_init() {
    terminal_state.input_pos = 0;
    terminal_state.input_buffer[0] = '\0';
    terminal_state.output_line_count = 0;
    terminal_state.cursor_visible = 1;
    terminal_state.show_prompt = 1;
    terminal_state.window_width_chars = 80;
    terminal_state.window_height_lines = 25;
    terminal_state.scroll_offset = 0;
    char prompt[64];
    gui_generate_prompt(prompt, sizeof(prompt));
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
void gui_execute(const char *input) {
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
        sys_cmd->func(args);
    } else {
        gui_terminal_print(cmd_name, TERM_COLOR_ERROR);
        gui_terminal_print(": command not found\n", TERM_COLOR_ERROR);
    }
}
void cmd_gui(const char* args) {
    (void)args;
    extern int gui_running;
    extern void gui_init(void);
    if (gui_running) {
        gui_terminal_print("GUI is already running!\n", 0xFF000000);
        return;
    }
    gui_init();
}
void gui_terminal_handle_key(char key) {
    if (key == '\b') {
        if (terminal_state.input_pos > 0) {
            terminal_state.input_pos--;
            terminal_state.input_buffer[terminal_state.input_pos] = '\0';
        }
    } else if (key == '\n') {
        terminal_state.input_buffer[terminal_state.input_pos] = '\0';
        char prompt[64];
        gui_generate_prompt(prompt, sizeof(prompt));
        char full_line[1024];
        str_copy(full_line, prompt);
        str_append(full_line, terminal_state.input_buffer);
        str_append(full_line, "\n");
        gui_terminal_print(full_line, TERM_COLOR_COMMAND);
        if (str_len(terminal_state.input_buffer) > 0) {
            gui_execute(terminal_state.input_buffer);
        }
        terminal_state.input_pos = 0;
        terminal_state.input_buffer[0] = '\0';
        terminal_state.scroll_offset = 0;
    } else if (key >= 32 && key <= 126) {
        if (terminal_state.input_pos < 511) {
            terminal_state.input_buffer[terminal_state.input_pos++] = key;
            terminal_state.input_buffer[terminal_state.input_pos] = '\0';
        }
    }
}
void gui_terminal_handle_scroll(int delta) {
    int max_scroll = terminal_state.output_line_count - terminal_state.window_height_lines;
    if (max_scroll < 0) max_scroll = 0;
    if (delta > 0) {
         terminal_state.scroll_offset += 3;
         if (terminal_state.scroll_offset > max_scroll) terminal_state.scroll_offset = max_scroll;
    } else {
         terminal_state.scroll_offset -= 3;
         if (terminal_state.scroll_offset < 0) terminal_state.scroll_offset = 0;
    }
}
void gui_terminal_handle_scrollbar_click(int x, int y) {
    if (!terminal_state.scrollbar_visible) return;
    if (x >= terminal_state.scrollbar_x && x < terminal_state.scrollbar_x + terminal_state.scrollbar_width &&
        y >= terminal_state.scrollbar_thumb_y && y < terminal_state.scrollbar_thumb_y + terminal_state.scrollbar_thumb_height) {
        terminal_state.scrollbar_dragging = 1;
        terminal_state.scrollbar_drag_start_y = y - terminal_state.scrollbar_thumb_y;
    } else if (x >= terminal_state.scrollbar_x && x < terminal_state.scrollbar_x + terminal_state.scrollbar_width &&
               y >= terminal_state.scrollbar_y && y < terminal_state.scrollbar_y + terminal_state.scrollbar_height) {
        int total_lines = terminal_state.output_line_count;
        int visible_lines = terminal_state.window_height_lines;
        int scrollable_lines = total_lines - visible_lines;
        if (scrollable_lines > 0) {
            int click_pos = y - terminal_state.scrollbar_y;
            int track_height = terminal_state.scrollbar_height;
            terminal_state.scroll_offset = (click_pos * scrollable_lines) / track_height;
            if (terminal_state.scroll_offset < 0) terminal_state.scroll_offset = 0;
            if (terminal_state.scroll_offset > scrollable_lines) terminal_state.scroll_offset = scrollable_lines;
        }
    }
}
void gui_terminal_handle_scrollbar_drag(int x, int y) {
    (void)x;
    if (!terminal_state.scrollbar_dragging || !terminal_state.scrollbar_visible) return;
    int total_lines = terminal_state.output_line_count;
    int visible_lines = terminal_state.window_height_lines;
    int scrollable_lines = total_lines - visible_lines;
    if (scrollable_lines <= 0) return;
    int track_height = terminal_state.scrollbar_height - terminal_state.scrollbar_thumb_height;
    int drag_pos = y - terminal_state.scrollbar_drag_start_y - terminal_state.scrollbar_y;
    if (drag_pos < 0) drag_pos = 0;
    if (drag_pos > track_height) drag_pos = track_height;
    terminal_state.scroll_offset = (drag_pos * scrollable_lines) / track_height;
    if (terminal_state.scroll_offset < 0) terminal_state.scroll_offset = 0;
    if (terminal_state.scroll_offset > scrollable_lines) terminal_state.scroll_offset = scrollable_lines;
}
void gui_terminal_handle_scrollbar_release() {
    terminal_state.scrollbar_dragging = 0;
}
void gui_terminal_draw(gui_window_t* window, int text_x, int text_y) {
    gui_terminal_update_window_size(window);
    gui_set_needs_redraw(1);
    int char_h = fm_get_char_height();
    int visible_lines = terminal_state.window_height_lines;
    int start_idx = 0;
    if (terminal_state.output_line_count > visible_lines) {
        start_idx = terminal_state.output_line_count - visible_lines - terminal_state.scroll_offset;
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
    if (terminal_state.scrollbar_visible && terminal_state.scrollbar_y < effective_window_bottom) {
        int scrollbar_bottom = terminal_state.scrollbar_y + terminal_state.scrollbar_height;
        int visible_scrollbar_height = (scrollbar_bottom > effective_window_bottom) ?
            effective_window_bottom - terminal_state.scrollbar_y : terminal_state.scrollbar_height;
        if (visible_scrollbar_height > 0) {
            draw_rect(terminal_state.scrollbar_x, terminal_state.scrollbar_y,
                      terminal_state.scrollbar_width, visible_scrollbar_height, 0xFF2C2C2C);
            if (terminal_state.scrollbar_thumb_y < effective_window_bottom &&
                terminal_state.scrollbar_thumb_y + terminal_state.scrollbar_thumb_height > terminal_state.scrollbar_y) {
                int visible_thumb_y = terminal_state.scrollbar_thumb_y;
                int visible_thumb_height = terminal_state.scrollbar_thumb_height;
                if (visible_thumb_y < terminal_state.scrollbar_y) {
                    visible_thumb_height -= (terminal_state.scrollbar_y - visible_thumb_y);
                    visible_thumb_y = terminal_state.scrollbar_y;
                }
                if (visible_thumb_y + visible_thumb_height > effective_window_bottom) {
                    visible_thumb_height = effective_window_bottom - visible_thumb_y;
                }
                if (visible_thumb_height > 0) {
                    u32 thumb_color = terminal_state.scrollbar_dragging ? 0xFF666666 : 0xFF888888;
                    draw_rect(terminal_state.scrollbar_x + 2, visible_thumb_y,
                              terminal_state.scrollbar_width - 4, visible_thumb_height, thumb_color);
                }
            }
        }
    }
    int cur_y = text_y;
    int line_idx = 0;
    int max_output_lines = actual_visible_lines - 1;
    for (int i = start_idx; i < terminal_state.output_line_count && line_idx < max_output_lines; i++) {
        if (cur_y + char_h > effective_window_bottom - char_h) break;
        char* line = terminal_state.output_buffer[i];
        u32 color = terminal_state.output_colors[i];
        int line_len = str_len(line);
        int display_chars = terminal_state.scrollbar_visible ?
            terminal_state.window_width_chars - 2 : terminal_state.window_width_chars;
        if (line_len > display_chars) {
            char clipped_line[display_chars + 1];
            for (int j = 0; j < display_chars; j++) {
                clipped_line[j] = line[j];
            }
            clipped_line[display_chars] = '\0';
            gui_draw_text_line_fast(clipped_line, text_x, cur_y, color);
        } else {
            gui_draw_text_line_fast(line, text_x, cur_y, color);
        }
        cur_y += char_h;
        line_idx++;
    }
    int input_y = window->y + window->height - char_h - 5;
    if (input_y >= cur_y && input_y >= window->y + WINDOW_TITLE_HEIGHT &&
        input_y < effective_window_bottom - 5) {
        char prompt[64];
        gui_generate_prompt(prompt, sizeof(prompt));
        char full_input[1024];
        str_copy(full_input, prompt);
        str_append(full_input, terminal_state.input_buffer);
        int max_display = terminal_state.scrollbar_visible ?
            terminal_state.window_width_chars - 2 : terminal_state.window_width_chars;
        if (str_len(full_input) > max_display) {
            char display_text[max_display + 1];
            for (int i = 0; i < max_display - 3; i++) {
                display_text[i] = full_input[i];
            }
            str_copy(display_text + max_display - 3, "...");
            gui_draw_text_line_fast(display_text, text_x, input_y, TERM_COLOR_PROMPT);
        } else {
            gui_draw_text_line_fast(prompt, text_x, input_y, TERM_COLOR_PROMPT);
            int prompt_pixels = str_len(prompt) * CHAR_WIDTH;
            gui_draw_text_line_fast(terminal_state.input_buffer,
                                   text_x + prompt_pixels, input_y, TERM_COLOR_DEFAULT);
        }
    }
}