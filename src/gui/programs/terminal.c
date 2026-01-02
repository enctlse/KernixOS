#include <gui/gui.h>
#include <string/string.h>
#include <kernel/graph/graphics.h>
#include <drivers/ps2/mouse/mouse.h>
#include <kernel/console/graph/dos.h>
#include <kernel/console/console.h>
#include <shared/config/user.h>
#define GUI_OUTPUT_LINES 20
#define GUI_OUTPUT_COLS 80
typedef struct {
    char input_buffer[256];
    int input_pos;
    char output_buffer[GUI_OUTPUT_LINES][GUI_OUTPUT_COLS];
    int output_line;
    int cursor_visible;
    int show_prompt;
} gui_terminal_state_t;
static gui_terminal_state_t terminal_state = {0};
extern char cwd[];
void gui_generate_prompt(char* buffer, size_t size) {
    str_copy(buffer, USER_NAME);
    str_append(buffer, "@");
    str_append(buffer, PC_NAME);
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
void gui_terminal_print(const char *text, u32 color) {
    (void)color;
    const char *p = text;
    while (*p) {
        if (*p == '\n') {
            terminal_state.output_line++;
            if (terminal_state.output_line >= GUI_OUTPUT_LINES) {
                for (int i = 1; i < GUI_OUTPUT_LINES; i++) {
                    str_copy(terminal_state.output_buffer[i-1], terminal_state.output_buffer[i]);
                }
                str_copy(terminal_state.output_buffer[GUI_OUTPUT_LINES-1], "");
                terminal_state.output_line = GUI_OUTPUT_LINES - 1;
            }
        } else {
            int line = terminal_state.output_line;
            int col = str_len(terminal_state.output_buffer[line]);
            if (col < GUI_OUTPUT_COLS - 1) {
                terminal_state.output_buffer[line][col] = *p;
                terminal_state.output_buffer[line][col + 1] = '\0';
            }
        }
        p++;
    }
}
void gui_terminal_init() {
    terminal_state.input_pos = 0;
    terminal_state.input_buffer[0] = '\0';
    terminal_state.output_line = 0;
    terminal_state.cursor_visible = 1;
    terminal_state.show_prompt = 1;
    for (int i = 0; i < GUI_OUTPUT_LINES; i++) {
        terminal_state.output_buffer[i][0] = '\0';
    }
    char prompt[64];
    gui_generate_prompt(prompt, sizeof(prompt));
    gui_terminal_print(prompt, GFX_BLACK);
}
void gui_terminal_clear() {
    for (int i = 0; i < GUI_OUTPUT_LINES; i++) {
        terminal_state.output_buffer[i][0] = '\0';
    }
    terminal_state.output_line = 0;
    terminal_state.input_pos = 0;
    terminal_state.input_buffer[0] = '\0';
    char prompt[64];
    gui_generate_prompt(prompt, sizeof(prompt));
    gui_terminal_print(prompt, GFX_BLACK);
}
void gui_terminal_draw(gui_window_t* window, int text_x, int text_y) {
    int line_y = text_y;
    int lines_shown = 0;
    char prompt[64];
    gui_generate_prompt(prompt, sizeof(prompt));
    int prompt_len = str_len(prompt);
    for (int i = 0; i < GUI_OUTPUT_LINES - 1 && lines_shown < GUI_OUTPUT_LINES - 1; i++) {
        if (terminal_state.output_buffer[i][0] != '\0') {
            if (str_starts_with(terminal_state.output_buffer[i], prompt)) {
                gui_draw_text_line_fast(prompt, text_x, line_y, 0xFF00FF00);
                gui_draw_text_line_fast(terminal_state.output_buffer[i] + prompt_len, text_x + (prompt_len * 8), line_y, 0xFFFFFFFF);
            } else {
                gui_draw_text_line_fast(terminal_state.output_buffer[i], text_x, line_y, 0xFFFFFFFF);
            }
            line_y += 16;
            lines_shown++;
        }
    }
    char current_line[GUI_OUTPUT_COLS];
    str_copy(current_line, prompt);
    str_append(current_line, terminal_state.input_buffer);
    gui_draw_text_line_fast(prompt, text_x, line_y, 0xFF00FF00);
    gui_draw_text_line_fast(terminal_state.input_buffer, text_x + (prompt_len * 8), line_y, 0xFFFFFFFF);
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
        gui_terminal_print(prompt, GFX_BLACK);
        gui_terminal_print(terminal_state.input_buffer, GFX_BLACK);
        gui_terminal_print("\n", GFX_BLACK);
        if (str_equals(terminal_state.input_buffer, "exit")) {
            gui_running = 0;
            extern void graphics_disable_double_buffering(void);
            extern void console_window_init(void);
            extern void cursor_draw(void);
            extern void mouse_set_callback(void (*cb)(int32_t, int32_t, uint8_t));
            extern void usb_mouse_set_callback(void (*cb)(int32_t, int32_t, uint8_t));
            extern void usb_keyboard_set_callback(void (*cb)(char));
            extern void console_handle_key(char);
            graphics_disable_double_buffering();
            console_window_init();
            cursor_draw();
            mouse_set_callback(NULL);
            usb_mouse_set_callback(NULL);
            usb_keyboard_set_callback(console_handle_key);
        } else {
            console_execute(terminal_state.input_buffer);
        }
        terminal_state.input_pos = 0;
        terminal_state.input_buffer[0] = '\0';
        gui_generate_prompt(prompt, sizeof(prompt));
        gui_terminal_print(prompt, GFX_BLACK);
    } else if (key >= 32 && key <= 126 && terminal_state.input_pos < sizeof(terminal_state.input_buffer) - 1) {
        terminal_state.input_buffer[terminal_state.input_pos++] = key;
        terminal_state.input_buffer[terminal_state.input_pos] = '\0';
    }
}
FHDR(cmd_gui) {
    (void)s;
    if (gui_running) {
        print("GUI already running!\n", GFX_YELLOW);
        return;
    }
    print("Starting graphical interface...\n", GFX_GREEN);
    gui_init();
    print("GUI started successfully!\n", GFX_GREEN);
    print("Use mouse/keyboard inside GUI. Type 'exit' in the terminal window to return.\n", GFX_CYAN);
}
FHDR(cmd_loadcursor) {
    if (!s || s[0] == '\0') {
        print("Usage: loadcursor <filename>\n", GFX_RED);
        print("File should be in /system/cursor/ directory\n", GFX_CYAN);
        return;
    }
    print("Loading cursor: ", GFX_CYAN);
    print(s, GFX_YELLOW);
    print("\n", GFX_CYAN);
    print("Cursor loading not implemented yet (needs file system)\n", GFX_RED);
    print("Using built-in cursor for now\n", GFX_YELLOW);
}