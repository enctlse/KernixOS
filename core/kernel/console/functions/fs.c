#include <kernel/console/console.h>
#include <file_systems/vfs/vfs.h>
#include <string/string.h>
#include <drivers/ps2/mouse/mouse.h>
#include <drivers/usb/usb_mouse.h>
#include <drivers/ps2/keyboard/keyboard.h>
extern char cwd[];
#define MAX_PATH_LEN 256
#include <gui/gui.h>
#include <gui/programs/programs.h>
FHDR(cmd_cat) {
    const char *path = s;
    if (!path || *path == '\0') {
        print("cat: missing file operand\n", GFX_RED);
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
        print("error: cannot open file: ", GFX_RED);
        print(full_path, GFX_RED);
        print("\n", GFX_RED);
        return;
    }
    char buf[256];
    ssize_t bytes;
    while ((bytes = fs_read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[bytes] = '\0';
        print(buf, GFX_WHITE);
    }
    fs_close(fd);
    buf[0] = '\0';
    print("\n", GFX_WHITE);
}
FHDR(cmd_ls) {
    const char *path = s;
    if (!s || *s == '\0') {
        path = cwd;
    }
    fs_node *dir = fs_resolve(path);
    if (!dir) {
        print("error: directory not found\n", GFX_RED);
        return;
    }
    if (dir->type != FS_DIR) {
        print("error: not a directory\n", GFX_RED);
        return;
    }
    fs_node *child = dir->children;
    if (!child) {
        print("this folder is empty \n", GFX_GRAY_50);
        return;
    }
    int count = 0;
    while (child) {
        u32 color = GFX_WHITE;
        const char *type_str = "";
        if (child->type == FS_DIR) {
            color = GFX_BLUE_50;
            type_str = "/";
        } else if (child->type == FS_DEV) {
            color = GFX_WHITE;
            type_str = "*";
            color = GFX_WHITE;
        }
        print(child->name, color);
        print(type_str, color);
        if (child->type == FS_FILE && child->size > 0) {
            char buf[32];
            str_copy(buf, " | ");
            str_append_uint(buf, (u32)child->size);
            str_append(buf, " bytes");
            print(buf, GFX_GRAY_50);
        }
        print("  ", GFX_WHITE);
        count++;
        if (count % 6 == 0) {
            print("\n", GFX_WHITE);
        }
        child = child->next;
    }
    if (count % 6 != 0) {
    print("\n", GFX_WHITE);
}
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
        print("cd: directory not found: ", GFX_RED);
        print(full_path, GFX_RED);
        print("\n", GFX_RED);
        return;
    }
    if (dir->type != FS_DIR) {
        print("cd: not a directory: ", GFX_RED);
        print(full_path, GFX_RED);
        print("\n", GFX_RED);
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
        print("mkdir: missing directory name\n", GFX_RED);
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
        print("mkdir: cannot create directory: ", GFX_RED);
        print(full_path, GFX_RED);
        print("\n", GFX_RED);
    }
}
FHDR(cmd_touch) {
    const char *path = s;
    if (!path || *path == '\0') {
        print("touch: missing file name\n", GFX_RED);
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
        print("touch: cannot create file: ", GFX_RED);
        print(full_path, GFX_RED);
        print("\n", GFX_RED);
        return;
    }
    fs_close(fd);
}
FHDR(cmd_mouse) {
    (void)s;
    int kb_interrupts = keyboard_get_interrupt_count();
    print("Keyboard interrupts: ", GFX_WHITE);
    char buf[16];
    str_copy(buf, "");
    str_append_uint(buf, (uint32_t)kb_interrupts);
    print(buf, GFX_YELLOW);
    print("\n", GFX_WHITE);
    int kb_has_key = keyboard_has_key();
    print("Keyboard buffer: ", GFX_WHITE);
    print(kb_has_key ? "HAS DATA" : "EMPTY", kb_has_key ? GFX_GREEN : GFX_RED);
    print("\n", GFX_WHITE);
    if (mouse_is_initialized()) {
        int32_t x, y;
        uint8_t buttons;
        int interrupts = mouse_get_interrupt_count();
        mouse_get_position(&x, &y);
        buttons = mouse_get_buttons();
        print("PS/2 Mouse position: X=", GFX_WHITE);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)x);
        print(buf, GFX_CYAN);
        print(" Y=", GFX_WHITE);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)y);
        print(buf, GFX_CYAN);
        print("\n", GFX_WHITE);
        print("PS/2 Buttons: ", GFX_WHITE);
        if (buttons & MOUSE_BUTTON_LEFT) print("LEFT ", GFX_GREEN);
        if (buttons & MOUSE_BUTTON_RIGHT) print("RIGHT ", GFX_GREEN);
        if (buttons & MOUSE_BUTTON_MIDDLE) print("MIDDLE ", GFX_GREEN);
        if (!buttons) print("NONE", GFX_GRAY_50);
        print("\n", GFX_WHITE);
        print("PS/2 Mouse interrupts: ", GFX_WHITE);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)interrupts);
        print(buf, GFX_YELLOW);
        print("\n", GFX_WHITE);
    } else {
        print("PS/2 Mouse not initialized\n", GFX_RED);
    }
    if (usb_mouse_is_initialized()) {
        int32_t usb_x, usb_y;
        uint8_t usb_buttons;
        int usb_interrupts = usb_mouse_get_interrupt_count();
        usb_mouse_get_position(&usb_x, &usb_y);
        usb_buttons = usb_mouse_get_buttons();
        print("USB Mouse position: X=", GFX_WHITE);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)usb_x);
        print(buf, GFX_PURPLE);
        print(" Y=", GFX_WHITE);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)usb_y);
        print(buf, GFX_PURPLE);
        print("\n", GFX_WHITE);
        print("USB Buttons: ", GFX_WHITE);
        if (usb_buttons & USB_MOUSE_BUTTON_LEFT) print("LEFT ", GFX_GREEN);
        if (usb_buttons & USB_MOUSE_BUTTON_RIGHT) print("RIGHT ", GFX_GREEN);
        if (usb_buttons & USB_MOUSE_BUTTON_MIDDLE) print("MIDDLE ", GFX_GREEN);
        if (!usb_buttons) print("NONE", GFX_GRAY_50);
        print("\n", GFX_WHITE);
        print("USB Mouse interrupts: ", GFX_WHITE);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)usb_interrupts);
        print(buf, GFX_YELLOW);
        print("\n", GFX_WHITE);
    } else {
        print("USB Mouse not initialized\n", GFX_RED);
    }
    if (str_equals(s, "test")) {
        mouse_enable_test_mode();
        usb_mouse_enable_test_mode();
        print("Test mode enabled - mice will move automatically\n", GFX_GREEN);
    }
    if (str_equals(s, "force")) {
        mouse_force_update();
        usb_mouse_force_update();
        print("Forced mouse cursor update\n", GFX_GREEN);
    }
    if (str_equals(s, "usbtest")) {
        usb_mouse_enable_test_mode();
        print("USB mouse test mode enabled\n", GFX_CYAN);
    }
    if (str_equals(s, "ps2test")) {
        mouse_enable_test_mode();
        print("PS/2 mouse test mode enabled\n", GFX_GREEN);
    }
    if (str_equals(s, "cursor")) {
        print("Reloading mouse cursor to arrow...\n", GFX_CYAN);
        mouse_reload_cursor();
        print("Cursor reloaded! Switch to GUI to see the arrow.\n", GFX_GREEN);
    }
    if (str_equals(s, "sensitivity")) {
        int current_div = mouse_get_sensitivity();
        char buf[16];
        str_copy(buf, "");
        str_append_uint(buf, current_div);
        print("Current mouse sensitivity divider: ", GFX_WHITE);
        print(buf, GFX_CYAN);
        print(" (higher = slower)\n", GFX_WHITE);
        print("Use 'mouse sensitivity <1-4>' to change\n", GFX_GRAY_70);
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
            print("Mouse sensitivity divider set to ", GFX_GREEN);
            char buf[16];
            str_copy(buf, "");
            str_append_uint(buf, new_div);
            print(buf, GFX_CYAN);
            print("\n", GFX_GREEN);
        } else {
            print("Invalid sensitivity value. Use 1-4 (higher = slower)\n", GFX_RED);
        }
    }
}
void cmd_mousemove(const char *args) {
    if (!mouse_is_initialized()) {
        print("Mouse not initialized\n", GFX_RED);
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
    print("Mouse moved to: X=", GFX_WHITE);
    printInt(x, GFX_CYAN);
    print(" Y=", GFX_WHITE);
    printInt(y, GFX_CYAN);
    print("\n", GFX_WHITE);
}
FHDR(cmd_cursor) {
    (void)s;
    print("Reloading mouse cursor from BMP files...\n", GFX_CYAN);
    mouse_reload_cursor();
    print("Cursor reloaded! Switch to GUI to see changes.\n", GFX_GREEN);
}