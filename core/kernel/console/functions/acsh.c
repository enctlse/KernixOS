#include <kernel/console/console.h>
#include <kernel/console/functions.h>
#include <kernel/graph/graphics.h>
#include <kernel/graph/fm.h>
#include <drivers/cmos/cmos.h>
#include <string/string.h>
#include <kernel/exceptions/timer.h>
#include <kernel/graph/theme.h>
static u32 banner_y = 0;
static u32 banner_y_s = BANNER_Y_SPACING;
static u8 last_second = 0;
static u8 needs_update = 1;
static u32 current_banner_height = BANNER_HEIGHT;
static void banner_timer_callback(void);
void banner_init(void) {}
void banner_draw(void) {}
void banner_update_time(void) {}
static void banner_timer_callback(void) {}
void banner_tick(void) {}
void banner_force_update(void) {}
u32 banner_get_height(void) { return 0; }
void console_window_init(void) {}
void console_window_clear(u32 color) {
    u32 fb_w = get_fb_width();
    u32 fb_h = get_fb_height();
    u32 banner_h = banner_get_height();
    draw_rect(0, banner_h, fb_w, fb_h - banner_h, color);
    banner_draw();
    cursor_x = CONSOLE_PADDING_X;
    cursor_y = banner_h;
}
u32 console_window_get_start_y(void) { return banner_get_height(); }
u32 console_window_get_max_y(void) { return get_fb_height(); }
void console_window_check_scroll(void) {}
void console_window_update_layout(void) {
    u32 banner_h = banner_get_height();
    if (cursor_y < banner_h) cursor_y = banner_h;
}
void console_window_scroll_lines(u32 lines) {
    if (lines == 0) return;
    u32 char_height = fm_get_char_height() * font_scale;
    u32 banner_h = banner_get_height();
    scroll_up(lines);
    u32 scroll_pixels = lines * char_height;
    if (cursor_y >= scroll_pixels) cursor_y -= scroll_pixels;
    else cursor_y = banner_h;
    if (graphics_is_double_buffering_enabled()) graphics_swap_buffers();
}
u32 console_window_get_visible_lines(void) {
    u32 char_height = fm_get_char_height() * font_scale;
    u32 fb_h = get_fb_height();
    u32 banner_h = banner_get_height();
    u32 available_height = fb_h - banner_h;
    return available_height / char_height;
}
int console_window_needs_scroll(void) {
    u32 char_height = fm_get_char_height() * font_scale;
    u32 fb_h = get_fb_height();
    return (cursor_y + char_height > fb_h) ? 1 : 0;
}
FHDR(cmd_echo)
{
    if (*s == '\0') {
        print("\n", GFX_GRAY_70);
        return;
    }
    print(s, GFX_GRAY_70);
}
FHDR(cmd_clear)
{
    if (*s != '\0') {
    print("Error: invalid option: ", GFX_RED);
    print(s, GFX_RED);
    print("\n", GFX_RED);
    return;
    }
    extern int gui_running;
    if (gui_running) {
        print("Clear command is disabled in GUI mode. Use 'cls' instead.\n", GFX_YELLOW);
        return;
    }
    u32 color = CONSOLESCREEN_BG_COLOR;
    if (*s != '\0') {
        return;
    }
    shell_clear_screen(color);
    banner_force_update();
}
FHDR(cmd_cls)
{
    if (*s != '\0') {
    print("Error: invalid option: ", GFX_RED);
    print(s, GFX_RED);
    print("\n", GFX_RED);
    return;
    }
    extern int gui_running;
    extern void gui_terminal_clear();
    if (gui_running) {
        gui_terminal_clear();
        return;
    }
    cmd_clear(s);
}
FHDR(cmd_fsize)
{
    if (*s != '\0') {
    print("Error: invalid option: ", GFX_RED);
    print(s, GFX_RED);
    print("\n", GFX_RED);
    return;
    }
    if (*s == '\0') {
        char buf[64];
        str_copy(buf, "Current font size: ");
        str_append_uint(buf, font_scale);
        print(buf, GFX_GRAY_70);
        return;
    }
    while (*s == ' ') s++;
    int size = 0;
    while (*s >= '0' && *s <= '9') {
        size = size * 10 + (*s - '0');
        s++;
    }
    if (size < 1 || size > 4) {
        print("Invalid size. Use 1-4\n", GFX_RED);
        return;
    }
    clear(CONSOLESCREEN_BG_COLOR);
    set_font_scale(size);
    clear(CONSOLESCREEN_BG_COLOR);
    banner_force_update();
    console_window_update_layout();
    cursor_x = CONSOLE_PADDING_X;
    cursor_y = banner_get_height();
}
FHDR(cmd_help)
{
    if (*s != '\0') {
    print("Error: invalid option: ", GFX_RED);
    print(s, GFX_RED);
    print("\n", GFX_RED);
    return;
    }
    if (*s == '\0') {
        print("acsh, version 1.0.0.0-release\n", GFX_GRAY_70);
        print("  echo [text]    - echo [text]\n", GFX_GRAY_70);
        print("  clear [color]  - clear screen\n", GFX_GRAY_70);
        print("  gui            - Launch the GUI\n", GFX_GRAY_70);
        print("  cls            - clear screen (works in GUI)\n", GFX_GRAY_70);
        print("  help [command] - displays this list\n", GFX_GRAY_70);
        print("  scale [1-4]    - change screen size\n", GFX_GRAY_70);
        print("  date           - show current date\n", GFX_GRAY_70);
        print("  time           - show current time\n", GFX_GRAY_70);
        print("  calendar       - show date & time\n", GFX_GRAY_70);
        print("  uptime         - displays the uptime\n", GFX_GRAY_70);
        print("  fetch          - KernixOS system info\n", GFX_GRAY_70);
        print("  cat <file>     - show file content\n", GFX_GRAY_70);
        print("  cd [path]      - Change the current directory\n", GFX_GRAY_70);
        print("  ls [path]      - list directory contents\n", GFX_GRAY_70);
        print("  mkdir          - Create the directory\n", GFX_GRAY_70);
        print("  touch          - Create the file\n", GFX_GRAY_70);
        print("  reboot         - reboot the system\n", GFX_GRAY_70);
        print("  meminfo        - heap memory information\n", GFX_GRAY_70);
        print("  cpuinfo        - heap cpu information\n", GFX_GRAY_70);
        print("  modules        - shows all modules in fs\n", GFX_GRAY_70);
        print("  history        - show command history\n", GFX_GRAY_70);
        print("Type 'help <command>' for details", GFX_CYAN);
    } else {
        const char *p = s;
        while (*p == ' ') p++;
        console_cmd_t *cmd = console_find_cmd(p);
        if (cmd) {
            char buf[128];
            str_copy(buf, "\n");
            str_append(buf, cmd->description);
            print(buf, GFX_GRAY_70);
            str_copy(buf, "\nUsage: ");
            str_append(buf, cmd->usage);
            print(buf, GFX_YELLOW);
            print("\n", GFX_GRAY_70);
        } else {
            print("\nCommand not found", GFX_RED);
        }
    }
}