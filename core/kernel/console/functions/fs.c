#include <kernel/console/console.h>
#include <fs/vfs/vfs.h>
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
        print("cat: missing file operand\n", red);
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
        print("error: cannot open file: ", red);
        print(full_path, red);
        print("\n", red);
        return;
    }
    char buf[256];
    ssize_t bytes;
    while ((bytes = fs_read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[bytes] = '\0';
        print(buf, white);
    }
    fs_close(fd);
    buf[0] = '\0';
    print("\n", white);
}
FHDR(cmd_ls) {
    const char *path = s;
    if (!s || *s == '\0') {
        path = cwd;
    }
    fs_node *dir = fs_resolve(path);
    if (!dir) {
        print("error: directory not found\n", red);
        return;
    }
    if (dir->type != FS_DIR) {
        print("error: not a directory\n", red);
        return;
    }
    fs_node *child = dir->children;
    if (!child) {
        print("this folder is empty \n", gray_50);
        return;
    }
    int count = 0;
    while (child) {
        u32 color = white;
        const char *type_str = "";
        if (child->type == FS_DIR) {
            color = blue_50;
            type_str = "/";
        } else if (child->type == FS_DEV) {
            color = white;
            type_str = "*";
            color = white;
        }
        print(child->name, color);
        print(type_str, color);
        if (child->type == FS_FILE && child->size > 0) {
            char buf[32];
            str_copy(buf, " | ");
            str_append_uint(buf, (u32)child->size);
            str_append(buf, " bytes");
            print(buf, gray_50);
        }
        print("  ", white);
        count++;
        if (count % 6 == 0) {
            print("\n", white);
        }
        child = child->next;
    }
    if (count % 6 != 0) {
    print("\n", white);
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
        print("mkdir: missing directory name\n", red);
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
        print("mkdir: cannot create directory: ", red);
        print(full_path, red);
        print("\n", red);
    }
}
FHDR(cmd_touch) {
    const char *path = s;
    if (!path || *path == '\0') {
        print("touch: missing file name\n", red);
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
        print("touch: cannot create file: ", red);
        print(full_path, red);
        print("\n", red);
        return;
    }
    fs_close(fd);
}
FHDR(cmd_mount) {
    const char *args = s;
    if (!args || *args == '\0') {
        print("mount: usage: mount <src> <tgt> <type>\n", red);
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
        print("mount: failed to mount ", red);
        print(src, red);
        print(" to ", red);
        print(tgt, red);
        print(" as ", red);
        print(type, red);
        print("\n", red);
    } else {
        print("mounted ", green);
        print(src, green);
        print(" to ", green);
        print(tgt, green);
        print(" as ", green);
        print(type, green);
        print("\n", green);
    }
}
FHDR(cmd_mouse) {
    (void)s;
    int kb_interrupts = keyboard_get_interrupt_count();
    print("Keyboard interrupts: ", white);
    char buf[16];
    str_copy(buf, "");
    str_append_uint(buf, (uint32_t)kb_interrupts);
    print(buf, yellow);
    print("\n", white);
    int kb_has_key = keyboard_has_key();
    print("Keyboard buffer: ", white);
    print(kb_has_key ? "HAS DATA" : "EMPTY", kb_has_key ? green : red);
    print("\n", white);
    if (mouse_is_initialized()) {
        int32_t x, y;
        uint8_t buttons;
        int interrupts = mouse_get_interrupt_count();
        mouse_get_position(&x, &y);
        buttons = mouse_get_buttons();
        print("PS/2 Mouse position: X=", white);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)x);
        print(buf, cyan);
        print(" Y=", white);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)y);
        print(buf, cyan);
        print("\n", white);
        print("PS/2 Buttons: ", white);
        if (buttons & MOUSE_BUTTON_LEFT) print("LEFT ", green);
        if (buttons & MOUSE_BUTTON_RIGHT) print("RIGHT ", green);
        if (buttons & MOUSE_BUTTON_MIDDLE) print("MIDDLE ", green);
        if (!buttons) print("NONE", gray_50);
        print("\n", white);
        print("PS/2 Mouse interrupts: ", white);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)interrupts);
        print(buf, yellow);
        print("\n", white);
    } else {
        print("PS/2 Mouse not initialized\n", red);
    }
    if (usb_mouse_is_initialized()) {
        int32_t usb_x, usb_y;
        uint8_t usb_buttons;
        int usb_interrupts = usb_mouse_get_interrupt_count();
        usb_mouse_get_position(&usb_x, &usb_y);
        usb_buttons = usb_mouse_get_buttons();
        print("USB Mouse position: X=", white);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)usb_x);
        print(buf, purple);
        print(" Y=", white);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)usb_y);
        print(buf, purple);
        print("\n", white);
        print("USB Buttons: ", white);
        if (usb_buttons & USB_MOUSE_BUTTON_LEFT) print("LEFT ", green);
        if (usb_buttons & USB_MOUSE_BUTTON_RIGHT) print("RIGHT ", green);
        if (usb_buttons & USB_MOUSE_BUTTON_MIDDLE) print("MIDDLE ", green);
        if (!usb_buttons) print("NONE", gray_50);
        print("\n", white);
        print("USB Mouse interrupts: ", white);
        str_copy(buf, "");
        str_append_uint(buf, (uint32_t)usb_interrupts);
        print(buf, yellow);
        print("\n", white);
    } else {
        print("USB Mouse not initialized\n", red);
    }
    if (str_equals(s, "test")) {
        mouse_enable_test_mode();
        usb_mouse_enable_test_mode();
        print("Test mode enabled - mice will move automatically\n", green);
    }
    if (str_equals(s, "force")) {
        mouse_force_update();
        usb_mouse_force_update();
        print("Forced mouse cursor update\n", green);
    }
    if (str_equals(s, "usbtest")) {
        usb_mouse_enable_test_mode();
        print("USB mouse test mode enabled\n", cyan);
    }
    if (str_equals(s, "ps2test")) {
        mouse_enable_test_mode();
        print("PS/2 mouse test mode enabled\n", green);
    }
    if (str_equals(s, "cursor")) {
        print("Reloading mouse cursor to arrow...\n", cyan);
        mouse_reload_cursor();
        print("Cursor reloaded! Switch to GUI to see the arrow.\n", green);
    }
    if (str_equals(s, "sensitivity")) {
        int current_div = mouse_get_sensitivity();
        char buf[16];
        str_copy(buf, "");
        str_append_uint(buf, current_div);
        print("Current mouse sensitivity divider: ", white);
        print(buf, cyan);
        print(" (higher = slower)\n", white);
        print("Use 'mouse sensitivity <1-4>' to change\n", gray_70);
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
            print("Mouse sensitivity divider set to ", green);
            char buf[16];
            str_copy(buf, "");
            str_append_uint(buf, new_div);
            print(buf, cyan);
            print("\n", green);
        } else {
            print("Invalid sensitivity value. Use 1-4 (higher = slower)\n", red);
        }
    }
}
void cmd_mousemove(const char *args) {
    if (!mouse_is_initialized()) {
        print("Mouse not initialized\n", red);
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
    print("Mouse moved to: X=", white);
    printInt(x, cyan);
    print(" Y=", white);
    printInt(y, cyan);
    print("\n", white);
}
FHDR(cmd_cursor) {
    (void)s;
    print("Reloading mouse cursor from BMP files...\n", cyan);
    mouse_reload_cursor();
    print("Cursor reloaded! Switch to GUI to see changes.\n", green);
}