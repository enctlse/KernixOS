#include <kernel/shell/acsh.h>
#include <kernel/shell/functions.h>
#include <string/string.h>
#include <fs/vfs/vfs.h>
#define MAX_LINES 100
#define MAX_LINE_LEN 256
static char lines[MAX_LINES][MAX_LINE_LEN];
static int line_count = 0;
static int edit_cursor_x = 0;
static int edit_cursor_y = 0;
static char filename[256] = "";
static int modified = 0;
int in_kedit = 0;
static void kedit_clear_screen() {
    shell_clear_screen(CONSOLESCREEN_BG_COLOR);
    banner_force_update();
}
static void kedit_draw_status() {
    u32 fb_w = get_fb_width();
    u32 fb_h = get_fb_height();
    u32 char_height = fm_get_char_height() * font_scale;
    u32 status_y = fb_h - char_height;
    draw_rect(0, status_y, fb_w, char_height, gray_20);
    char status[256];
    str_copy(status, "KEDIT - ");
    str_append(status, filename);
    if (modified) str_append(status, " *");
    str_append(status, " | Line: ");
    str_append_uint(status, edit_cursor_y + 1);
    str_append(status, " Col: ");
    str_append_uint(status, edit_cursor_x + 1);
    str_append(status, " | Ctrl+S Save, Ctrl+Q Quit");
    u32 cursor_x_save = cursor_x;
    u32 cursor_y_save = cursor_y;
    cursor_x = 0;
    cursor_y = status_y / char_height;
    string(status, white);
    cursor_x = cursor_x_save;
    cursor_y = cursor_y_save;
}
static void kedit_draw() {
    kedit_clear_screen();
    u32 char_height = fm_get_char_height() * font_scale;
    u32 banner_h = banner_get_height();
    int start_y = banner_h;
    u32 line_spacing = 1;
    for (int i = 0; i < line_count; i++) {
        cursor_x = 0;
        cursor_y = (start_y / char_height) + i + line_spacing;
        string(lines[i], gray_70);
    }
    kedit_draw_status();
    cursor_x = edit_cursor_x;
    cursor_y = (start_y / char_height) + edit_cursor_y + line_spacing;
    cursor_draw();
}
static void kedit_load(const char* fname) {
    str_copy(filename, fname);
    int fd = fs_open(fname, O_RDONLY);
    if (fd < 0) {
        line_count = 1;
        str_copy(lines[0], "");
        return;
    }
    char buf[1024];
    int bytes_read;
    int line_idx = 0;
    int buf_pos = 0;
    while ((bytes_read = fs_read(fd, buf, sizeof(buf))) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            if (buf[i] == '\n' || line_idx >= MAX_LINES - 1) {
                lines[line_idx][buf_pos] = '\0';
                line_idx++;
                buf_pos = 0;
                if (line_idx >= MAX_LINES) break;
            } else if (buf_pos < MAX_LINE_LEN - 1) {
                lines[line_idx][buf_pos++] = buf[i];
            }
        }
        if (line_idx >= MAX_LINES) break;
    }
    if (buf_pos > 0 && line_idx < MAX_LINES) {
        lines[line_idx][buf_pos] = '\0';
        line_idx++;
    }
    line_count = line_idx;
    if (line_count == 0) {
        line_count = 1;
        str_copy(lines[0], "");
    }
    fs_close(fd);
    modified = 0;
}
static void kedit_save() {
    if (!filename[0]) return;
    int fd = fs_open(filename, O_WRONLY | O_CREAT);
    if (fd < 0) {
        print("Error: Cannot save file\n", red);
        return;
    }
    for (int i = 0; i < line_count; i++) {
        fs_write(fd, lines[i], str_len(lines[i]));
        if (i < line_count - 1) {
            fs_write(fd, "\n", 1);
        }
    }
    fs_close(fd);
    modified = 0;
    cursor_x = 0;
    cursor_y = get_fb_height() / (fm_get_char_height() * font_scale) - 2;
    string("File saved successfully", green);
}
static void kedit_insert_char(char c) {
    if (edit_cursor_x >= MAX_LINE_LEN - 1) return;
    char* line = lines[edit_cursor_y];
    int len = str_len(line);
    if (len >= MAX_LINE_LEN - 1) return;
    for (int i = len; i >= edit_cursor_x; i--) {
        line[i + 1] = line[i];
    }
    line[edit_cursor_x] = c;
    edit_cursor_x++;
    modified = 1;
}
static void kedit_delete_char() {
    char* line = lines[edit_cursor_y];
    int len = str_len(line);
    if (edit_cursor_x > 0 && len > 0) {
        for (int i = edit_cursor_x - 1; i < len; i++) {
            line[i] = line[i + 1];
        }
        edit_cursor_x--;
    } else if (edit_cursor_x == 0 && edit_cursor_y > 0) {
        int prev_len = str_len(lines[edit_cursor_y - 1]);
        if (prev_len + len < MAX_LINE_LEN - 1) {
            str_append(lines[edit_cursor_y - 1], line);
            for (int i = edit_cursor_y; i < line_count - 1; i++) {
                str_copy(lines[i], lines[i + 1]);
            }
            line_count--;
            edit_cursor_y--;
            edit_cursor_x = prev_len;
        }
    }
    modified = 1;
}
static void kedit_new_line() {
    if (line_count >= MAX_LINES) return;
    char* current = lines[edit_cursor_y];
    int current_len = str_len(current);
    if (edit_cursor_x <= current_len) {
        for (int i = line_count; i > edit_cursor_y + 1; i--) {
            str_copy(lines[i], lines[i - 1]);
        }
        char rest[MAX_LINE_LEN];
        str_copy(rest, &current[edit_cursor_x]);
        current[edit_cursor_x] = '\0';
        str_copy(lines[edit_cursor_y + 1], rest);
        line_count++;
        edit_cursor_y++;
        edit_cursor_x = 0;
        modified = 1;
    }
}
static void kedit_move_cursor(int dx, int dy) {
    edit_cursor_x += dx;
    edit_cursor_y += dy;
    if (edit_cursor_y < 0) edit_cursor_y = 0;
    if (edit_cursor_y >= line_count) edit_cursor_y = line_count - 1;
    if (edit_cursor_x < 0) edit_cursor_x = 0;
    if (edit_cursor_x > str_len(lines[edit_cursor_y])) edit_cursor_x = str_len(lines[edit_cursor_y]);
}
void kedit_handle_key(int key) {
    if (key == 17) {
        in_kedit = 0;
        shell_clear_screen(CONSOLESCREEN_BG_COLOR);
        banner_force_update();
        shell_print_prompt();
        return;
    }
    if (key == 19) {
        kedit_save();
        kedit_draw();
        return;
    }
    if (key == '\n' || key == '\r') {
        kedit_new_line();
    } else if (key == 8 || key == 127) {
        kedit_delete_char();
    } else if (key >= 32 && key <= 126) {
        kedit_insert_char((char)key);
    } else if (key == 0x80) {
        kedit_move_cursor(0, -1);
    } else if (key == 0x81) {
        kedit_move_cursor(0, 1);
    } else if (key == 0x82) {
        kedit_move_cursor(-1, 0);
    } else if (key == 0x83) {
        kedit_move_cursor(1, 0);
    } else {
        return;
    }
    kedit_draw();
}
FHDR(cmd_kedit) {
    if (!s || !*s) {
        print("Usage: kedit <filename>\n", red);
        return;
    }
    shell_clear_screen(CONSOLESCREEN_BG_COLOR);
    banner_force_update();
    edit_cursor_x = 0;
    edit_cursor_y = 0;
    modified = 0;
    kedit_load(s);
    kedit_draw();
    in_kedit = 1;
}