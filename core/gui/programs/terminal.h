#pragma once
struct gui_window;
typedef struct gui_window gui_window_t;
#define GUI_OUTPUT_LINES 100
#define GUI_OUTPUT_COLS 256
#define MAX_LINE_LENGTH 512
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 16
#define TERM_COLOR_DEFAULT  0xFFFFFFFF
#define TERM_COLOR_PROMPT   0xFFFFFFFF
#define TERM_COLOR_ERROR    0xFFFF0000
#define TERM_COLOR_COMMAND  0xFFFFFFFF
#define TERM_COLOR_SUCCESS  0xFFFFFFFF
#define TERM_COLOR_INFO     0xFFFFFFFF
#define TERM_COLOR_INPUT    0xFFFFFFFF
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
    int cursor_blink_timer;
} gui_terminal_state_t;
extern gui_terminal_state_t* current_terminal_state;
void gui_terminal_init(gui_terminal_state_t* state);
void gui_terminal_clear(gui_terminal_state_t* state);
void gui_terminal_print(gui_terminal_state_t* state, const char *text, u32 color);
void gui_terminal_draw(gui_terminal_state_t* state, gui_window_t* window, int text_x, int text_y);
void gui_terminal_handle_key(gui_terminal_state_t* state, char key);
void gui_terminal_handle_scroll(gui_terminal_state_t* state, int delta);
void gui_terminal_handle_scrollbar_click(gui_terminal_state_t* state, int x, int y);
void gui_terminal_handle_scrollbar_drag(gui_terminal_state_t* state, int x, int y);
void gui_terminal_handle_scrollbar_release(gui_terminal_state_t* state);
void gui_terminal_update_window_size(gui_terminal_state_t* state, gui_window_t* window);
void gui_terminal_add_line(gui_terminal_state_t* state, const char* line, u32 color);
void gui_execute(gui_terminal_state_t* state, const char *input);
void terminal_print(const char *text, u32 color);