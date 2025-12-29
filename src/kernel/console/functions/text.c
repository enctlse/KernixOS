#include <kernel/console/console.h>
FHDR(cmd_echo)
{
    if (*s == '\0') {
        print("\n", GFX_WHITE);
        return;
    }
    print(s, GFX_WHITE);
}
FHDR(cmd_clear)
{
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
    if (*s == '\0') {
        char buf[64];
        str_copy(buf, "Current font size: ");
        str_append_uint(buf, font_scale);
        print(buf, GFX_WHITE);
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
    if (*s == '\0') {
        print("  echo [text]    - echo [text]\n", GFX_WHITE);
        print("  clear [color]  - clear screen\n", GFX_WHITE);
        print("  gui            - Launch the GUI\n", GFX_WHITE);
        print("  cls            - clear screen (works in GUI)\n", GFX_WHITE);
        print("  help [command] - displays this list\n", GFX_WHITE);
        print("  scale [1-4]    - change screen size\n", GFX_WHITE);
        print("  date           - show current date\n", GFX_WHITE);
        print("  time           - show current time\n", GFX_WHITE);
        print("  calendar       - show date & time\n", GFX_WHITE);
        print("  uptime         - displays the uptime\n", GFX_WHITE);
        print("  fetch          - DystopiaOS system info\n", GFX_WHITE);
        print("  cat <file>     - show file content\n", GFX_WHITE);
        print("  cd [path]      - Change the current directory\n", GFX_WHITE);
        print("  ls [path]      - list directory contents\n", GFX_WHITE);
        print("  mkdir          - Create the directory\n", GFX_WHITE);
        print("  touch          - Create the file\n", GFX_WHITE);
        print("  reboot         - reboot the system\n", GFX_WHITE);
        print("  meminfo        - heap memory information\n", GFX_WHITE);
        print("  modules        - shows all modules in fs\n", GFX_WHITE);
        print("  history        - show command history\n", GFX_WHITE);
        print("Type 'help <command>' for details", GFX_CYAN);
    } else {
        const char *p = s;
        while (*p == ' ') p++;
        console_cmd_t *cmd = console_find_cmd(p);
        if (cmd) {
            char buf[128];
            str_copy(buf, "\n");
            str_append(buf, cmd->description);
            print(buf, GFX_WHITE);
            str_copy(buf, "\nUsage: ");
            str_append(buf, cmd->usage);
            print(buf, GFX_YELLOW);
            print("\n", GFX_WHITE);
        } else {
            print("\nCommand not found", GFX_RED);
        }
    }
}