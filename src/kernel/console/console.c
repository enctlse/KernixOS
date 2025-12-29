#include "console.h"
#include "functions.h"
#include <string/string.h>
#include <kernel/gui/gui.h>
static char input_buffer[MAX_INPUT_LEN];
static int input_pos = 0;
int boot_completed = 0;
#define MAX_HISTORY 50
static char command_history[MAX_HISTORY][MAX_INPUT_LEN];
static int history_count = 0;
static int history_pos = -1;
#define MAX_PATH_LEN 256
char cwd[MAX_PATH_LEN] = "/";  
static int cmd_count = 24;
void cmd_history(const char *args);
static console_cmd_t commands[MAX_CMDS] = {
    CMDENTRY(cmd_echo, "echo", "Prints text to console", "echo [text]"),
    CMDENTRY(cmd_clear, "clear", "Clear the screen", "clear screen"),
    CMDENTRY(cmd_help, "help", "Displays all available commands", "help [command]"),
    CMDENTRY(cmd_fsize, "scale", "Change screen size", "scale [2-4]"),
    CMDENTRY(cmd_modules, "modules", "List loaded modules", "modules"),
    CMDENTRY(cmd_sysinfo, "fetch", "Display fetch", "fetch"),
    CMDENTRY(cmd_cal, "calendar", "Displays current date & time", "calendar"),
    CMDENTRY(cmd_date, "date", "Displays current date", "date"),
    CMDENTRY(cmd_uptime, "uptime", "System uptime", "uptime"),
    CMDENTRY(cmd_time, "time", "Displays current time", "time"),
    CMDENTRY(cmd_cat, "cat", "show file", "cat <file>"),
    CMDENTRY(cmd_ls, "ls", "list directory contents", "ls [path]"),
    CMDENTRY(cmd_cd, "cd", "Change directory", "cd [path]"),
    CMDENTRY(cmd_mkdir, "mkdir", "Create directory", "mkdir <name>"),
    CMDENTRY(cmd_touch, "touch", "Create empty file", "touch <name>"),
    CMDENTRY(cmd_poweroff, "poweroff", "Power off the system", "poweroff"),
    CMDENTRY(cmd_reboot, "reboot", "Reboot the system", "reboot"),
    CMDENTRY(cmd_shutdown, "shutdown", "Shutdown the system", "shutdown"),
    CMDENTRY(cmd_gui, "gui", "Start graphical interface", "gui"),
    CMDENTRY(cmd_loadcursor, "loadcursor", "Load BMP cursor", "loadcursor <filename>"),
    CMDENTRY(cmd_mouse, "mouse", "Show mouse status", "mouse [test|ps2test|usbtest|force|cursor|sensitivity|sensitivity <1-4>]"),
    CMDENTRY(cmd_mousemove, "mousemove", "Move mouse cursor", "mousemove <x> <y>"),
    CMDENTRY(cmd_history, "history", "Show command history", "history"),
    CMDENTRY(cmd_cls, "cls", "Clear screen (works in GUI)", "cls"),
};
static void add_to_history(const char *command) {
    if (str_len(command) == 0) return;
    if (history_count > 0 && str_equals(command_history[history_count - 1], command)) {
        return;
    }
    if (history_count < MAX_HISTORY) {
        str_copy(command_history[history_count], command);
        history_count++;
    } else {
        for (int i = 1; i < MAX_HISTORY; i++) {
            str_copy(command_history[i - 1], command_history[i]);
        }
        str_copy(command_history[MAX_HISTORY - 1], command);
    }
}
static const char* get_history_command(int pos) {
    if (pos >= 0 && pos < history_count) {
        return command_history[pos];
    }
    return NULL;
}
static void reset_history_pos() {
    history_pos = -1;
}
void cmd_history(const char *args) {
    if (history_count == 0) {
        print("No commands in history.\n", GFX_YELLOW);
        return;
    }
    print("Command history:\n", GFX_CYAN);
    for (int i = 0; i < history_count; i++) {
        char num_buf[8];
        str_copy(num_buf, "");
        str_append_uint(num_buf, i + 1);
        str_append(num_buf, ": ");
        print(num_buf, GFX_GREEN);
        print(command_history[i], GFX_WHITE);
        print("\n", GFX_WHITE);
    }
}
static int console_module_init(void) {
    return 0;
}
static void console_module_fini(void) {
}
driver_module console_module = (driver_module) {
    .name = "console",
    .mount = "/dev/console",
    .version = VERSION_NUM(0, 1, 2, 0),
    .init = console_module_init,
    .fini = console_module_fini,
    .open = NULL,
    .read = NULL,
    .write = NULL,
};
void console_init(void)
{
    input_pos = 0;
    input_buffer[0] = '\0';
    sconsole_theme(THEME_FLU);
    banner_init();
    console_window_init();
    cursor_();
    cursor_draw();
}
void console_run(void)
{
}
void console_handle_key(int c)
{
    if (gui_running) {
        return;
    }
    cursor_c();
    if (c == '\n') {
        putchar('\n', GFX_WHITE);
        if (input_pos > 0) {
            input_buffer[input_pos] = '\0';
            add_to_history(input_buffer);
            reset_history_pos();
            int has_chain = 0;
            for (int i = 0; i < input_pos - 1; i++) {
                if (input_buffer[i] == '&' && input_buffer[i+1] == '&') {
                    has_chain = 1;
                    break;
                }
            }
            if (has_chain) {
                parse_and_execute_chained(input_buffer);
            } else {
                console_execute(input_buffer);
            }
            input_pos = 0;
            input_buffer[0] = '\0';
        }
        cursor_reset_blink();
        if (!gui_running && boot_completed) {
            shell_print_prompt();
        }
        if (graphics_is_double_buffering_enabled()) {
            graphics_swap_buffers();
        }
        console_window_check_scroll();
        graphics_swap_buffers();
        return;
    }
    if (c == '\r') {
        putchar('\n', GFX_WHITE);
        input_buffer[input_pos++] = '\n';
        cursor_draw();
        return;
    }
    if (c == '\b') {
        if (input_pos > 0) {
            input_pos--;
            input_buffer[input_pos] = '\0';
            u32 char_width = fm_get_char_width() * font_scale;
            u32 char_height = fm_get_char_height() * font_scale;
            if (cursor_x >= char_width) {
                cursor_x -= char_width;
                putchar(' ', GFX_WHITE);
                cursor_x -= char_width;
                draw_rect(cursor_x, cursor_y, char_width, char_height, CONSOLESCREEN_BG_COLOR);
            }
        }
        cursor_reset_blink();
        cursor_draw();
        graphics_swap_buffers();
        return;
    }
    if (c == '\t') {
        for (int i = 0; i < 4; i++) {
            if (input_pos < MAX_INPUT_LEN - 1) {
                input_buffer[input_pos++] = ' ';
                input_buffer[input_pos] = '\0';
            }
            putchar(' ', GFX_WHITE);
        }
        cursor_reset_blink();
        cursor_draw();
        return;
    }
    if (c == 0x80) {
        if (history_pos == -1) {
            input_buffer[input_pos] = '\0';
        }
        if (history_pos < history_count - 1) {
            history_pos++;
            const char *cmd = get_history_command(history_pos);
            if (cmd) {
                while (input_pos > 0) {
                    input_pos--;
                    cursor_x -= fm_get_char_width() * font_scale;
                }
                draw_rect(cursor_x, cursor_y, fb_width - cursor_x, fm_get_char_height() * font_scale, CONSOLESCREEN_BG_COLOR);
                str_copy(input_buffer, cmd);
                input_pos = str_len(cmd);
                string(cmd, GFX_WHITE);
            }
        }
        cursor_draw();
        return;
    }
    if (c == 0x81) {
        if (history_pos > 0) {
            history_pos--;
            const char *cmd = get_history_command(history_pos);
            if (cmd) {
                while (input_pos > 0) {
                    input_pos--;
                    cursor_x -= fm_get_char_width() * font_scale;
                }
                draw_rect(cursor_x, cursor_y, fb_width - cursor_x, fm_get_char_height() * font_scale, CONSOLESCREEN_BG_COLOR);
                str_copy(input_buffer, cmd);
                input_pos = str_len(cmd);
                string(cmd, GFX_WHITE);
            }
        } else if (history_pos == 0) {
            history_pos = -1;
            while (input_pos > 0) {
                input_pos--;
                cursor_x -= fm_get_char_width() * font_scale;
            }
            draw_rect(cursor_x, cursor_y, fb_width - cursor_x, fm_get_char_height() * font_scale, CONSOLESCREEN_BG_COLOR);
        }
        cursor_draw();
        return;
    }
    if (history_pos != -1 && ((c >= 32 && c <= 126) || c == ' ')) {
        reset_history_pos();
    }
    console_window_check_scroll();
    if (input_pos < MAX_INPUT_LEN - 1) {
        input_buffer[input_pos++] = c;
        input_buffer[input_pos] = '\0';
        putchar(c, GFX_WHITE);
    } else {
        putchar(c, GFX_WHITE);
    }
    cursor_reset_blink();
    cursor_draw();
    if (graphics_is_double_buffering_enabled()) {
        graphics_swap_buffers();
    }
}
void console_execute(const char *input)
{
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
    console_cmd_t *cmd = NULL;
    if (cmd_name[0] != '\0') {
        cmd = console_find_cmd(cmd_name);
    } else {
    }
    if (cmd) {
        print("\n", GFX_CYAN);
        cmd->func(args);
        banner_force_update();
        console_window_check_scroll();
        graphics_swap_buffers();
    } else {
        print(cmd_name, GFX_RED);
        print(": command not found", GFX_RED);
        console_window_check_scroll();
        graphics_swap_buffers();
    }
}
console_cmd_t* console_find_cmd(const char *name)
{
    for (int i = 0; i < cmd_count; i++) {
        const char *a = name;
        const char *b = commands[i].name;
        if (!b) {
            continue;
        }
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
            return &commands[i];
        }
    }
    return NULL;
}