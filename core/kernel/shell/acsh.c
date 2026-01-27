#include "acsh.h"
#include "functions.h"
#include <string/string.h>
#include <gui/gui.h>
static char input_buffer[MAX_INPUT_LEN];
static int input_pos = 0;
static int input_start_x = 0;
int boot_completed = 0;
#define MAX_HISTORY 50
static char command_history[MAX_HISTORY][MAX_INPUT_LEN];
static int history_count = 0;
static int history_pos = -1;
#define MAX_PATH_LEN 256
char cwd[MAX_PATH_LEN] = "/";  
static int cmd_count = 28;
void cmd_history(const char *args);
static console_cmd_t commands[MAX_CMDS] = {
    CMDENTRY(cmd_echo, "echo", "Prints text to console", "echo [text]"),
    CMDENTRY(cmd_clear, "clear", "Clear the screen", "clear screen"),
    CMDENTRY(cmd_help, "help", "Displays all available commands", "help [command]"),
    CMDENTRY(cmd_modules, "modules", "List loaded modules", "modules"),
    CMDENTRY(cmd_sysinfo, "fetch", "Display fetch", "fetch"),
    CMDENTRY(cmd_cpuinfo, "cpuinfo", "Display CPU information", "cpuinfo"),
    CMDENTRY(cmd_cal, "calendar", "Displays current date & time", "calendar"),
    CMDENTRY(cmd_date, "date", "Displays current date", "date"),
    CMDENTRY(cmd_uptime, "uptime", "System uptime", "uptime"),
    CMDENTRY(cmd_time, "time", "Displays current time", "time"),
    CMDENTRY(cmd_cat, "cat", "show file", "cat <file>"),
    CMDENTRY(cmd_ls, "ls", "list directory contents", "ls [path]"),
    CMDENTRY(cmd_cd, "cd", "Change directory", "cd [path]"),
    CMDENTRY(cmd_mkdir, "mkdir", "Create directory", "mkdir <name>"),
    CMDENTRY(cmd_touch, "touch", "Create empty file", "touch <name>"),
    CMDENTRY(cmd_mount, "mount", "Mount filesystem", "mount <src> <tgt> <type>"),
    CMDENTRY(cmd_poweroff, "poweroff", "Power off the system", "poweroff"),
    CMDENTRY(cmd_reboot, "reboot", "Reboot the system", "reboot"),
    CMDENTRY(cmd_shutdown, "shutdown", "Shutdown the system", "shutdown"),
    CMDENTRY(cmd_gui, "gui", "Start graphical interface", "gui"),
    CMDENTRY(cmd_history, "history", "Show command history", "history"),
    CMDENTRY(cmd_cls, "cls", "Clear screen (works in GUI)", "cls"),
    CMDENTRY(cmd_kedit, "kedit", "Simple text editor", "kedit <filename>"),
    CMDENTRY(cmd_ping, "ping", "Ping a host", "ping <host>"),
    CMDENTRY(cmd_insmod, "insmod", "Load a kernel module", "insmod <path>"),
    CMDENTRY(cmd_rmmod, "rmmod", "Unload a kernel module", "rmmod <name>"),
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
        print("No commands in history.\n", yellow);
        return;
    }
    print("Command history:\n", cyan);
    for (int i = 0; i < history_count; i++) {
        char num_buf[8];
        str_copy(num_buf, "");
        str_append_uint(num_buf, i + 1);
        str_append(num_buf, ": ");
        print(num_buf, green);
        print(command_history[i], gray_70);
        print("\n", gray_70);
    }
}
static int console_handler_startup(void) {
    return 0;
}
static void console_handler_shutdown(void) {
}
struct component_handler console_handler = {
    .identifier = "console",
    .attachment_point = "/dev/console",
    .build_number = BUILD_VERSION(0, 1, 2, 0),
    .startup = console_handler_startup,
    .shutdown = console_handler_shutdown,
    .access = NULL,
    .retrieve = NULL,
    .store = NULL,
};
void console_init(void)
{
    input_pos = 0;
    input_buffer[0] = '\0';
    sconsole_theme(THEME_FLU);
    banner_init();
    console_window_init();
    graphics_disable_double_buffering();
    cursor_();
    cursor_draw();
}
void console_run(void)
{
}
void console_set_input_start_x(void) {
    input_start_x = cursor_x;
}
static void redraw_input_line() {
    cursor_x = input_start_x;
    u32 char_height = fm_get_char_height() * font_scale;
    draw_rect(cursor_x, cursor_y, fb_width - cursor_x, char_height, CONSOLESCREEN_BG_COLOR);
    string(input_buffer, gray_70);
    cursor_x = input_start_x + input_pos * fm_get_char_width() * font_scale;
    cursor_draw();
    if (!gui_running && graphics_is_double_buffering_enabled()) {
        graphics_swap_buffers();
    }
}
void console_handle_key(int c)
{
    if (gui_running) {
        return;
    }
    if (in_kedit) {
        kedit_handle_key(c);
        return;
    }
    cursor_c();
    if (c == '\n') {
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
        input_start_x = cursor_x;
        if (!gui_running && graphics_is_double_buffering_enabled()) {
            graphics_swap_buffers();
        }
        console_window_check_scroll();
        if (!gui_running) {
            graphics_swap_buffers();
        }
        return;
    }
    if (c == '\r') {
        putchar('\n', gray_70);
        input_buffer[input_pos++] = '\n';
        cursor_draw();
        return;
    }
    if (c == 3) {
    input_pos = 0;
    input_buffer[0] = '\0';
    cursor_reset_blink();
    shell_print_prompt();
    return;
    }
    if (c == '\b') {
        if (input_pos > 0) {
            input_pos--;
            for (int i = input_pos; i <= str_len(input_buffer); i++) {
                input_buffer[i] = input_buffer[i+1];
            }
            redraw_input_line();
        }
        cursor_reset_blink();
        return;
    }
    if (c == '\t') {
        for (int i = 0; i < 4; i++) {
            if (str_len(input_buffer) < MAX_INPUT_LEN - 1) {
                for (int j = str_len(input_buffer); j >= input_pos; j--) {
                    input_buffer[j+1] = input_buffer[j];
                }
                input_buffer[input_pos] = ' ';
                input_pos++;
            }
        }
        redraw_input_line();
        cursor_reset_blink();
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
                str_copy(input_buffer, cmd);
                input_pos = str_len(cmd);
                redraw_input_line();
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
                str_copy(input_buffer, cmd);
                input_pos = str_len(cmd);
                redraw_input_line();
            }
        } else if (history_pos == 0) {
            history_pos = -1;
            input_buffer[0] = '\0';
            input_pos = 0;
            redraw_input_line();
        }
        cursor_draw();
        return;
    }
    if (c == 0x82) {
        if (input_pos > 0) {
            input_pos--;
            cursor_x -= fm_get_char_width() * font_scale;
            cursor_draw();
            if (!gui_running && graphics_is_double_buffering_enabled()) {
                graphics_swap_buffers();
            }
        }
        return;
    }
    if (c == 0x83) {
        if (input_pos < str_len(input_buffer)) {
            input_pos++;
            cursor_x += fm_get_char_width() * font_scale;
            cursor_draw();
            if (!gui_running && graphics_is_double_buffering_enabled()) {
                graphics_swap_buffers();
            }
        }
        return;
    }
    if (c == 0x84) {
        input_pos = 0;
        redraw_input_line();
        return;
    }
    if (c == 0x85) {
        input_pos = str_len(input_buffer);
        redraw_input_line();
        return;
    }
    if (history_pos != -1 && ((c >= 32 && c <= 126) || c == ' ')) {
        reset_history_pos();
    }
    console_window_check_scroll();
    if (c >= 32 && c <= 126) {
        if (str_len(input_buffer) < MAX_INPUT_LEN - 1) {
            for (int i = str_len(input_buffer); i >= input_pos; i--) {
                input_buffer[i+1] = input_buffer[i];
            }
            input_buffer[input_pos] = c;
            input_pos++;
            redraw_input_line();
        } else {
            putchar(c, gray_70);
        }
        cursor_reset_blink();
        return;
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
        putchar('\n', gray_70);
        putchar('\n', gray_70);
        cmd->func(args);
        banner_force_update();
        console_window_check_scroll();
        if (!gui_running) {
            graphics_swap_buffers();
        }
    } else {
        putchar('\n', gray_70);
        print(cmd_name, red);
        print(": command not found", red);
        console_window_check_scroll();
        if (!gui_running) {
            graphics_swap_buffers();
        }
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