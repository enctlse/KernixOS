#include <kernel/shell/acsh.h>
#include <fs/vfs/vfs.h>
#include <string/string.h>
#include <drivers/ps2/mouse/mouse.h>
#include <drivers/usb/usb_mouse.h>
#include <drivers/ps2/keyboard/keyboard.h>
#include <gui/programs/terminal.h>
extern char cwd[];
#define MAX_PATH_LEN 256
#include <gui/gui.h>
#include <gui/programs/programs.h>
FHDR(cmd_cat) {
    const char *path = s;
    if (!path || *path == '\0') {
        terminal_print("cat: missing file operand\n", TERM_COLOR_ERROR);
        return;
    }
    char full_path[MAX_PATH_LEN];
    if (*path == '/') {
        str_copy(full_path, path);
    } else {
        str_copy(full_path, cwd);
        if (cwd[str_len(cwd) - 1] != '/') {
            str_append(full_path, "/");
        }
        str_append(full_path, path);
    }
    int fd = fs_open(full_path, O_RDONLY);
    if (fd < 0) {
        terminal_print("error: cannot open file: ", TERM_COLOR_ERROR);
        terminal_print(full_path, TERM_COLOR_ERROR);
        terminal_print("\n", TERM_COLOR_ERROR);
        return;
    }
    char buf[256];
    ssize_t bytes;
    while ((bytes = fs_read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[bytes] = '\0';
        terminal_print(buf, TERM_COLOR_DEFAULT);
    }
    fs_close(fd);
    buf[0] = '\0';
    terminal_print("\n", TERM_COLOR_DEFAULT);
}
FHDR(cmd_ls) {
    const char *path = s;
    if (!s || *s == '\0') {
        path = cwd;
    }
    fs_node *dir = fs_resolve(path);
    if (!dir) {
        terminal_print("error: directory not found\n", TERM_COLOR_ERROR);
        return;
    }
    if (dir->type != FS_DIR) {
        terminal_print("error: not a directory\n", TERM_COLOR_ERROR);
        return;
    }
    fs_node *child = dir->children;

    while (child) {
        u32 color = TERM_COLOR_DEFAULT;
        const char *type_str = "";
        if (child->type == FS_DIR) {
            color = TERM_COLOR_INFO;
            type_str = "/";
        } else if (child->type == FS_DEV) {
            color = TERM_COLOR_DEFAULT;
            type_str = "*";
        }
        terminal_print(child->name, color);
        terminal_print(type_str, color);
        if (child->type == FS_FILE && child->size > 0) {
            char buf[32];
            str_copy(buf, " | ");
            str_append_uint(buf, (u32)child->size);
            str_append(buf, " bytes");
            terminal_print(buf, TERM_COLOR_INFO);
        }
        terminal_print("  ", TERM_COLOR_DEFAULT);
        child = child->next;
    }
    terminal_print("\n", TERM_COLOR_DEFAULT);
}
FHDR(cmd_cd) {
    const char *path = s;
    if (!path || *path == '\0') {
        path = "/";
    }
    if (str_equals(path, "..")) {
        char *last_slash = cwd;
        for (char *p = cwd; *p; p++) {
            if (*p == '/') last_slash = p;
        }
        if (last_slash != cwd) {
            *last_slash = '\0';
        } else {
            str_copy(cwd, "/");
        }
        return;
    }
    char full_path[MAX_PATH_LEN];
    if (*path == '/') {
        str_copy(full_path, path);
    } else {
        str_copy(full_path, cwd);
        if (cwd[str_len(cwd) - 1] != '/') {
            str_append(full_path, "/");
        }
        str_append(full_path, path);
    }
    fs_node *dir = fs_resolve(full_path);
    if (!dir) {
        print("cd: directory not found: ", red);
        print(full_path, red);
        print("\n", red);
        return;
    }
    if (dir->type != FS_DIR) {
        print("cd: not a directory: ", red);
        print(full_path, red);
        print("\n", red);
        return;
    }
    str_copy(cwd, full_path);
    if (str_len(cwd) > 1 && cwd[str_len(cwd) - 1] != '/') {
        str_append(cwd, "/");
    }
}
FHDR(cmd_mkdir) {
    const char *path = s;
    if (!path || *path == '\0') {
        terminal_print("mkdir: missing directory name\n", TERM_COLOR_ERROR);
        return;
    }
    char full_path[MAX_PATH_LEN];
    if (*path == '/') {
        str_copy(full_path, path);
    } else {
        str_copy(full_path, cwd);
        if (cwd[str_len(cwd) - 1] != '/') {
            str_append(full_path, "/");
        }
        str_append(full_path, path);
    }
    if (fs_mkdir(full_path) < 0) {
        terminal_print("mkdir: cannot create directory: ", TERM_COLOR_ERROR);
        terminal_print(full_path, TERM_COLOR_ERROR);
        terminal_print("\n", TERM_COLOR_ERROR);
    }
}
FHDR(cmd_touch) {
    const char *path = s;
    if (!path || *path == '\0') {
        terminal_print("touch: missing file name\n", TERM_COLOR_ERROR);
        return;
    }
    char full_path[MAX_PATH_LEN];
    if (*path == '/') {
        str_copy(full_path, path);
    } else {
        str_copy(full_path, cwd);
        if (cwd[str_len(cwd) - 1] != '/') {
            str_append(full_path, "/");
        }
        str_append(full_path, path);
    }
    int fd = fs_open(full_path, O_CREAT | O_WRONLY);
    if (fd < 0) {
        terminal_print("touch: cannot create file: ", TERM_COLOR_ERROR);
        terminal_print(full_path, TERM_COLOR_ERROR);
        terminal_print("\n", TERM_COLOR_ERROR);
        return;
    }
    fs_close(fd);
}
FHDR(cmd_mount) {
    const char *args = s;
    if (!args || *args == '\0') {
        terminal_print("mount: usage: mount <src> <tgt> <type>\n", TERM_COLOR_ERROR);
        return;
    }
    char src[64], tgt[64], type[64];
    int i = 0;
    // Parse src
    while (*args && *args != ' ') {
        src[i++] = *args++;
    }
    src[i] = '\0';
    while (*args == ' ') args++;
    i = 0;
    // Parse tgt
    while (*args && *args != ' ') {
        tgt[i++] = *args++;
    }
    tgt[i] = '\0';
    while (*args == ' ') args++;
    i = 0;
    // Parse type
    while (*args && *args != ' ') {
        type[i++] = *args++;
    }
    type[i] = '\0';
    if (fs_mount(src, tgt, type) < 0) {
        terminal_print("mount: failed to mount ", TERM_COLOR_ERROR);
        terminal_print(src, TERM_COLOR_ERROR);
        terminal_print(" to ", TERM_COLOR_ERROR);
        terminal_print(tgt, TERM_COLOR_ERROR);
        terminal_print(" as ", TERM_COLOR_ERROR);
        terminal_print(type, TERM_COLOR_ERROR);
        terminal_print("\n", TERM_COLOR_ERROR);
    } else {
        terminal_print("mounted ", TERM_COLOR_SUCCESS);
        terminal_print(src, TERM_COLOR_SUCCESS);
        terminal_print(" to ", TERM_COLOR_SUCCESS);
        terminal_print(tgt, TERM_COLOR_SUCCESS);
        terminal_print(" as ", TERM_COLOR_SUCCESS);
        terminal_print(type, TERM_COLOR_SUCCESS);
        terminal_print("\n", TERM_COLOR_SUCCESS);
    }
}
FHDR(cmd_mouse) {
    (void)s;
    int kb_interrupts = keyboard_get_interrupt_count();
    terminal_print("Keyboard interrupts: ", TERM_COLOR_DEFAULT);
    char buf[16];
    str_copy(buf, "");
    str_append_uint(buf, (uint32_t)kb_interrupts);
    terminal_print(buf, TERM_COLOR_COMMAND);
    terminal_print("\n", TERM_COLOR_DEFAULT);
    int kb_has_key = keyboard_has_key();
    terminal_print("Keyboard buffer: ", TERM_COLOR_DEFAULT);
    terminal_print(kb_has_key ? "HAS DATA" : "EMPTY", kb_has_key ? TERM_COLOR_SUCCESS : TERM_COLOR_ERROR);
    terminal_print("\n", TERM_COLOR_DEFAULT);
    if (mouse_is_initialized()) {
        int32_t x, y;
        uint8_t buttons;
        int interrupts = mouse_get_interrupt_count();
        mouse_get_position(&x, &y);
        buttons = mouse_get_buttons();
        terminal_print("PS/2 Mouse position: X=", TERM_COLOR_DEFAULT);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)x);
        terminal_print(buf, TERM_COLOR_INFO);
        terminal_print(" Y=", TERM_COLOR_DEFAULT);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)y);
        terminal_print(buf, TERM_COLOR_INFO);
        terminal_print("\n", TERM_COLOR_DEFAULT);
        terminal_print("PS/2 Buttons: ", TERM_COLOR_DEFAULT);
        if (buttons & MOUSE_BUTTON_LEFT) terminal_print("LEFT ", TERM_COLOR_SUCCESS);
        if (buttons & MOUSE_BUTTON_RIGHT) terminal_print("RIGHT ", TERM_COLOR_SUCCESS);
        if (buttons & MOUSE_BUTTON_MIDDLE) terminal_print("MIDDLE ", TERM_COLOR_SUCCESS);
        if (!buttons) terminal_print("NONE", TERM_COLOR_INFO);
        terminal_print("\n", TERM_COLOR_DEFAULT);
        terminal_print("PS/2 Mouse interrupts: ", TERM_COLOR_DEFAULT);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)interrupts);
        terminal_print(buf, TERM_COLOR_COMMAND);
        terminal_print("\n", TERM_COLOR_DEFAULT);
    } else {
        terminal_print("PS/2 Mouse not initialized\n", TERM_COLOR_ERROR);
    }
    if (usb_mouse_is_initialized()) {
        int32_t usb_x, usb_y;
        uint8_t usb_buttons;
        int usb_interrupts = usb_mouse_get_interrupt_count();
        usb_mouse_get_position(&usb_x, &usb_y);
        usb_buttons = usb_mouse_get_buttons();
        terminal_print("USB Mouse position: X=", TERM_COLOR_DEFAULT);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)usb_x);
        terminal_print(buf, TERM_COLOR_INFO);
        terminal_print(" Y=", TERM_COLOR_DEFAULT);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)usb_y);
        terminal_print(buf, TERM_COLOR_INFO);
        terminal_print("\n", TERM_COLOR_DEFAULT);
        terminal_print("USB Buttons: ", TERM_COLOR_DEFAULT);
        if (usb_buttons & USB_MOUSE_BUTTON_LEFT) terminal_print("LEFT ", TERM_COLOR_SUCCESS);
        if (usb_buttons & USB_MOUSE_BUTTON_RIGHT) terminal_print("RIGHT ", TERM_COLOR_SUCCESS);
        if (usb_buttons & USB_MOUSE_BUTTON_MIDDLE) terminal_print("MIDDLE ", TERM_COLOR_SUCCESS);
        if (!usb_buttons) terminal_print("NONE", TERM_COLOR_INFO);
        terminal_print("\n", TERM_COLOR_DEFAULT);
        terminal_print("USB Mouse interrupts: ", TERM_COLOR_DEFAULT);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)usb_interrupts);
        terminal_print(buf, TERM_COLOR_COMMAND);
        terminal_print("\n", TERM_COLOR_DEFAULT);
    } else {
        terminal_print("USB Mouse not initialized\n", TERM_COLOR_ERROR);
    }
    if (str_equals(s, "test")) {
        mouse_enable_test_mode();
        usb_mouse_enable_test_mode();
        terminal_print("Test mode enabled - mice will move automatically\n", TERM_COLOR_SUCCESS);
    }
    if (str_equals(s, "force")) {
        mouse_force_update();
        usb_mouse_force_update();
        terminal_print("Forced mouse cursor update\n", TERM_COLOR_SUCCESS);
    }
    if (str_equals(s, "usbtest")) {
        usb_mouse_enable_test_mode();
        terminal_print("USB mouse test mode enabled\n", TERM_COLOR_INFO);
    }
    if (str_equals(s, "ps2test")) {
        mouse_enable_test_mode();
        terminal_print("PS/2 mouse test mode enabled\n", TERM_COLOR_SUCCESS);
    }
    if (str_equals(s, "cursor")) {
        terminal_print("Reloading mouse cursor to arrow...\n", TERM_COLOR_INFO);
        mouse_reload_cursor();
        terminal_print("Cursor reloaded! Switch to GUI to see the arrow.\n", TERM_COLOR_SUCCESS);
    }
    if (str_equals(s, "sensitivity")) {
        int current_div = mouse_get_sensitivity();
        char buf[16];
        str_copy(buf, "");
        str_append_uint(buf, current_div);
        terminal_print("Current mouse sensitivity divider: ", TERM_COLOR_DEFAULT);
        terminal_print(buf, TERM_COLOR_INFO);
        terminal_print(" (higher = slower)\n", TERM_COLOR_DEFAULT);
        terminal_print("Use 'mouse sensitivity <1-4>' to change\n", TERM_COLOR_INFO);
    }
    if (str_starts_with(s, "sensitivity ")) {
        const char* value_str = s + 12;
        int new_div = 0;
        for (int i = 0; value_str[i]; i++) {
            char c = value_str[i];
            if (c >= '0' && c <= '9') {
                new_div = new_div * 10 + (c - '0');
            }
        }
        if (new_div >= 1 && new_div <= 4) {
            mouse_set_sensitivity(new_div);
            terminal_print("Mouse sensitivity divider set to ", TERM_COLOR_SUCCESS);
            char buf[16];
            str_copy(buf, "");
            str_append_uint(buf, new_div);
            terminal_print(buf, TERM_COLOR_INFO);
            terminal_print("\n", TERM_COLOR_SUCCESS);
        } else {
            terminal_print("Invalid sensitivity value. Use 1-4 (higher = slower)\n", TERM_COLOR_ERROR);
        }
    }
}
void cmd_mousemove(const char *args) {
    if (!mouse_is_initialized()) {
        terminal_print("Mouse not initialized\n", TERM_COLOR_ERROR);
        return;
    }
    int x = 0, y = 0;
    const char *p = args;
    while (*p == ' ') p++;
    while (*p >= '0' && *p <= '9') {
        x = x * 10 + (*p - '0');
        p++;
    }
    while (*p == ' ') p++;
    while (*p >= '0' && *p <= '9') {
        y = y * 10 + (*p - '0');
        p++;
    }
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    mouse_set_position(x, y);
    terminal_print("Mouse moved to: X=", TERM_COLOR_DEFAULT);
    char buf[16];
    str_copy(buf, "");
    str_append_uint(buf, (uint32_t)x);
    terminal_print(buf, TERM_COLOR_INFO);
    terminal_print(" Y=", TERM_COLOR_DEFAULT);
    str_copy(buf, "");
    str_append_uint(buf, (uint32_t)y);
    terminal_print(buf, TERM_COLOR_INFO);
    terminal_print("\n", TERM_COLOR_DEFAULT);
}
FHDR(cmd_cursor) {
    (void)s;
    terminal_print("Reloading mouse cursor from BMP files...\n", TERM_COLOR_INFO);
    mouse_reload_cursor();
    terminal_print("Cursor reloaded! Switch to GUI to see changes.\n", TERM_COLOR_SUCCESS);
}