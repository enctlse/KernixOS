#include <gui/gui.h>
#include <string/string.h>
#include <kernel/graph/graphics.h>
#include <drivers/ps2/mouse/mouse.h>
#include <drivers/usb/usb_mouse.h>
#include <kernel/console/functions.h>
#include <kernel/console/console.h>
#include <kernel/exceptions/timer.h>
#include <file_systems/vfs/vfs.h>
#include <kernel/mem/klime/klime.h>
#include <gui/programs/terminal.h>
#include <drivers/cmos/cmos.h>
gui_state_t gui_state = {0};
int gui_running = 0;
int gui_mode = 0;
static int gui_cursor_visible = 1;
static int gui_key_pressed = 0;
static int gui_initialized = 0;
static int gui_needs_redraw = 1;
static int mouse_cursor_needs_redraw = 0;
static int gui_frame_count = 0;
#define FRAME_TIME_MS 16
#define VSYNC_WAIT_MS 16
static dirty_rect_t dirty_rects[MAX_DIRTY_RECTS];
static int dirty_rect_count = 0;
extern void* fs_klime;
#define MENU_BG_WHITE   0xFFFFFFFF
#define MENU_HIGHLIGHT  0xFFE0E0E0
#define MENU_TEXT_MAIN  0xFF333333
#define MENU_TEXT_DIM   0xFF888888
#define MENU_BORDER     0xFFCCCCCC
#define MENU_HOVER      0xFFF0F0F0
#define MENU_SEPARATOR  0xFFDDDDDD
#define MENU_SECTION_BG 0xFFF8F8F8
void draw_rounded_rect(int x, int y, int w, int h, int r, u32 color) {
    draw_rect(x + r, y, w - 2 * r, h, color);
    draw_rect(x, y + r, w, h - 2 * r, color);
    draw_circle(x + r, y + r, r, color);
    draw_circle(x + w - r, y + r, r, color);
    draw_circle(x + r, y + h - r, r, color);
    draw_circle(x + w - r, y + h - r, r, color);
}
static void launch_terminal_new(void) {
    gui_window_t* win = gui_create_window("Terminal", 50, 50, 600, 400);
    if (win) {
        win->is_terminal = 1;
        if (gui_state.taskbar_button_count < MAX_WINDOWS) {
            gui_state.taskbar_buttons[gui_state.taskbar_button_count].window_id = win->id;
            str_copy(gui_state.taskbar_buttons[gui_state.taskbar_button_count].text, win->title);
            gui_state.taskbar_button_count++;
        }
    }
}
static void launch_shutdown_new(void) {
    gui_create_window("Shutdown", 400, 300, 300, 200);
}
static void launch_calculator_new(void) {
    gui_create_window("Calculator", 200, 200, 300, 400);
}
static void launch_paint_new(void) {
    gui_create_window("Paint", 300, 100, 500, 400);
}
static void launch_text_editor_new(void) {
    gui_create_window("Text Editor", 300, 100, 500, 400);
}
void gui_draw_start_menu(gui_window_t* window) {
    if (!window || !str_equals(window->title, "Start Menu")) return;
    int mx = window->x;
    int my = window->y;
    int mw = window->width;
    int mh = window->height;
    draw_rect(mx, my, mw, mh, MENU_BG_WHITE);
    draw_rect(mx, my, mw, 1, MENU_BORDER);
    draw_rect(mx, my, 1, mh, MENU_BORDER);
    draw_rect(mx + mw - 1, my, 1, mh, MENU_BORDER);
    draw_rect(mx, my + mh - 1, mw, 1, MENU_BORDER);
    int current_y = my + 10;
    int item_h = 30;
    int item_spacing = 2;
    int section_padding = 8;
    draw_rect(mx + 5, current_y, mw - 10, 4 * item_h + 2 * section_padding, MENU_SECTION_BG);
    draw_rect(mx + 5, current_y + section_padding - 2, mw - 10, 1, MENU_SEPARATOR);
    gui_draw_text_line_fast("System", mx + 45, current_y + 2, MENU_TEXT_DIM);
    current_y += section_padding;
    gui_draw_text_line_fast("Terminal", mx + 45, current_y + 8, MENU_TEXT_MAIN);
    draw_rect(mx + 10, current_y + 5, 20, 20, 0xFF2E86C1);
    current_y += item_h + item_spacing;
    gui_draw_text_line_fast("File Manager", mx + 45, current_y + 8, MENU_TEXT_MAIN);
    draw_rect(mx + 10, current_y + 5, 20, 20, 0xFF27AE60);
    current_y += item_h + item_spacing;
    gui_draw_text_line_fast("Web Browser", mx + 45, current_y + 8, MENU_TEXT_MAIN);
    draw_rect(mx + 10, current_y + 5, 20, 20, 0xFFE74C3C);
    current_y += item_h + item_spacing;
    current_y += section_padding;
    draw_rect(mx + 5, current_y, mw - 10, item_h + 2 * section_padding, MENU_SECTION_BG);
    draw_rect(mx + 5, current_y + section_padding - 2, mw - 10, 1, MENU_SEPARATOR);
    gui_draw_text_line_fast("Settings", mx + 15, current_y + 2, MENU_TEXT_DIM);
    current_y += section_padding;
    gui_draw_text_line_fast("Settings", mx + 45, current_y + 8, MENU_TEXT_MAIN);
    draw_rect(mx + 10, current_y + 5, 20, 20, 0xFF9B59B6);
    current_y += item_h + item_spacing;
    current_y += section_padding;
    draw_rect(mx + 5, current_y, mw - 10, 2 * item_h + 2 * section_padding, MENU_SECTION_BG);
    draw_rect(mx + 5, current_y + section_padding - 2, mw - 10, 1, MENU_SEPARATOR);
    gui_draw_text_line_fast("Application & Multimedia", mx + 15, current_y + 2, MENU_TEXT_DIM);
    current_y += section_padding;
    gui_draw_text_line_fast("Text Editor", mx + 45, current_y + 8, MENU_TEXT_MAIN);
    draw_rect(mx + 10, current_y + 5, 20, 20, 0xFF3498DB);
    current_y += item_h + item_spacing;
    gui_draw_text_line_fast("Media Player", mx + 45, current_y + 8, MENU_TEXT_MAIN);
    draw_rect(mx + 10, current_y + 5, 20, 20, 0xFFF1C40F);
    current_y += item_h + item_spacing;
    current_y += section_padding;
    draw_rect(mx + 5, current_y, mw - 10, item_h + 2 * section_padding, MENU_SECTION_BG);
    draw_rect(mx + 5, current_y + section_padding - 2, mw - 10, 1, MENU_SEPARATOR);
    gui_draw_text_line_fast("System", mx + 15, current_y + 2, MENU_TEXT_DIM);
    current_y += section_padding;
    gui_draw_text_line_fast("Log out", mx + 45, current_y + 8, MENU_TEXT_MAIN);
    draw_rect(mx + 10, current_y + 5, 20, 20, 0xFFE67E22);
    current_y += item_h + item_spacing;
}
    static void on_start_button_click(gui_button_t* button);
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} __attribute__((packed)) bmp_file_header_t;
typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t important_colors;
} __attribute__((packed)) bmp_info_header_t;
bmp_image_t* gui_load_bmp(const char* path) {
    int fd = fs_open(path, O_RDONLY);
    if (fd < 0) {
        print("Failed to open BMP file: ", GFX_RED);
        print(path, GFX_RED);
        print("\n", GFX_RED);
        return NULL;
    }
    bmp_file_header_t file_header;
    bmp_info_header_t info_header;
    ssize_t read_bytes = fs_read(fd, &file_header, sizeof(bmp_file_header_t));
    if (read_bytes != (ssize_t)sizeof(bmp_file_header_t) || file_header.type != 0x4D42) {
        print("Invalid BMP file header\n", GFX_RED);
        fs_close(fd);
        return NULL;
    }
    read_bytes = fs_read(fd, &info_header, sizeof(bmp_info_header_t));
    if (read_bytes != (ssize_t)sizeof(bmp_info_header_t)) {
        print("Failed to read BMP info header\n", GFX_RED);
        fs_close(fd);
        return NULL;
    }
    if (info_header.bpp != 32) {
        print("Unsupported BMP format. Need 32-bit RGBA\n", GFX_RED);
        fs_close(fd);
        return NULL;
    }
    bmp_image_t* image = (bmp_image_t*)klime_alloc((klime_t*)fs_klime, sizeof(bmp_image_t), 1);
    if (!image) {
        print("Failed to allocate BMP image\n", GFX_RED);
        fs_close(fd);
        return NULL;
    }
    image->width = info_header.width;
    image->height = info_header.height;
    image->pixels = (u32*)klime_alloc((klime_t*)fs_klime, sizeof(u32), info_header.width * info_header.height);
    if (!image->pixels) {
        print("Failed to allocate BMP pixels\n", GFX_RED);
        klime_free((klime_t*)fs_klime, (u64*)image);
        fs_close(fd);
        return NULL;
    }
    uint32_t data_offset = file_header.offset;
    uint8_t skip_buf[1024];
    while (data_offset > 0) {
        size_t to_skip = data_offset > 1024 ? 1024 : data_offset;
        read_bytes = fs_read(fd, skip_buf, to_skip);
        if (read_bytes <= 0) break;
        data_offset -= read_bytes;
    }
    uint32_t bmp_pixels[info_header.width * info_header.height];
    read_bytes = fs_read(fd, bmp_pixels, (size_t)info_header.width * info_header.height * 4);
    fs_close(fd);
    if (read_bytes != (ssize_t)((size_t)info_header.width * info_header.height * 4)) {
        print("Failed to read BMP pixel data\n", GFX_RED);
        gui_free_bmp(image);
        return NULL;
    }
    for (int y = 0; y < info_header.height; y++) {
        for (int x = 0; x < info_header.width; x++) {
            uint32_t bmp_pixel = bmp_pixels[(info_header.height - 1 - y) * info_header.width + x];
            uint8_t b = (bmp_pixel >> 0) & 0xFF;
            uint8_t g = (bmp_pixel >> 8) & 0xFF;
            uint8_t r = (bmp_pixel >> 16) & 0xFF;
            uint8_t a = (bmp_pixel >> 24) & 0xFF;
            image->pixels[y * info_header.width + x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    print("Loaded BMP: ", GFX_GREEN);
    print(path, GFX_GREEN);
    char buf[32];
    str_copy(buf, "");
    str_append_uint(buf, image->width);
    str_append(buf, "x");
    str_append_uint(buf, image->height);
    print(buf, GFX_GREEN);
    print("\n", GFX_GREEN);
    return image;
}
void gui_free_bmp(bmp_image_t* image) {
    if (!image) return;
    if (image->pixels) {
        klime_free((klime_t*)fs_klime, (u64*)image->pixels);
    }
    klime_free((klime_t*)fs_klime, (u64*)image);
}
void gui_mark_dirty(int x, int y, int width, int height) {
    if (dirty_rect_count >= MAX_DIRTY_RECTS) {
        gui_needs_redraw = 1;
        return;
    }
    for (int i = 0; i < dirty_rect_count; i++) {
        dirty_rect_t* rect = &dirty_rects[i];
        if (rect->x <= x + width + 2 && rect->x + rect->width + 2 >= x &&
            rect->y <= y + height + 2 && rect->y + rect->height + 2 >= y) {
            int new_x = rect->x < x ? rect->x : x;
            int new_y = rect->y < y ? rect->y : y;
            int new_right = (rect->x + rect->width) > (x + width) ?
                           (rect->x + rect->width) : (x + width);
            int new_bottom = (rect->y + rect->height) > (y + height) ?
                            (rect->y + rect->height) : (y + height);
            rect->x = new_x;
            rect->y = new_y;
            rect->width = new_right - new_x;
            rect->height = new_bottom - new_y;
            return;
        }
    }
    dirty_rects[dirty_rect_count].x = x;
    dirty_rects[dirty_rect_count].y = y;
    dirty_rects[dirty_rect_count].width = width;
    dirty_rects[dirty_rect_count].height = height;
    dirty_rect_count++;
}
void gui_clear_dirty_rects() {
    dirty_rect_count = 0;
}
void gui_redraw_region(int x, int y, int width, int height) {
    draw_rect(x, y, width, height, DESKTOP_COLOR);
    for (int i = 0; i < gui_state.window_count; i++) {
        gui_window_t* window = &gui_state.windows[i];
        if (window->visible && window->state != WINDOW_STATE_MINIMIZED) {
            int win_left = window->x;
            int win_right = window->x + window->width;
            int win_top = window->y;
            int win_bottom = window->y + window->height;
            if (win_left < x + width && win_right > x &&
                win_top < y + height && win_bottom > y) {
                gui_draw_window(window);
            }
        }
    }
    for (int i = 0; i < gui_state.button_count; i++) {
        gui_button_t* button = &gui_state.buttons[i];
        int btn_left = button->x;
        int btn_right = button->x + button->width;
        int btn_top = button->y;
        int btn_bottom = button->y + button->height;
        if (btn_left < x + width && btn_right > x &&
            btn_top < y + height && btn_bottom > y) {
            gui_draw_button(button);
        }
    }
}
void gui_draw_text_fast(const char* str, int x, int y, u32 color) {
    static u32 cached_glyph_height = 0;
    if (cached_glyph_height == 0) {
        cached_glyph_height = fm_get_char_height();
    }
    const char* line_start = str;
    int current_y = y;
    while (*line_start) {
        const char* line_end = line_start;
        while (*line_end && *line_end != '\n') line_end++;
        char temp = *line_end;
        *(char*)line_end = '\0';
        if (line_start != line_end) {
            gui_draw_text_line_fast(line_start, x, current_y, color);
        }
        *(char*)line_end = temp;
        current_y += (int)cached_glyph_height + 2;
        line_start = line_end + (*line_end == '\n' ? 1 : 0);
    }
}
gui_window_t* gui_create_window(const char* title, int x, int y, int width, int height) {
    if (gui_state.window_count >= MAX_WINDOWS) return NULL;
    gui_window_t* window = &gui_state.windows[gui_state.window_count++];
    window->id = gui_state.next_window_id++;
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    str_copy(window->title, title);
    window->visible = 1;
    window->focused = 1;
    window->is_terminal = 0;
    for (int i = 0; i < gui_state.window_count - 1; i++)
        gui_state.windows[i].focused = 0;
    gui_needs_redraw = 1;
    return window;
}
void gui_destroy_window(gui_window_t* window) {
    if (!window) return;
    for (int i = 0; i < gui_state.taskbar_button_count; i++) {
        if (gui_state.taskbar_buttons[i].window_id == window->id) {
            for (int j = i; j < gui_state.taskbar_button_count - 1; j++) {
                gui_state.taskbar_buttons[j] = gui_state.taskbar_buttons[j + 1];
            }
            gui_state.taskbar_button_count--;
            break;
        }
    }
    for (int i = 0; i < gui_state.window_count; i++) {
        if (&gui_state.windows[i] == window) {
            for (int j = i; j < gui_state.window_count - 1; j++) {
                gui_state.windows[j] = gui_state.windows[j + 1];
            }
            gui_state.window_count--;
            break;
        }
    }
    gui_needs_redraw = 1;
}
void gui_focus_window(gui_window_t* window) {
    if (!window) return;
    if (gui_state.focused_window_id == window->id) {
        return;
    }
    int window_index = -1;
    for (int i = 0; i < gui_state.window_count; i++) {
        if (&gui_state.windows[i] == window) {
            window_index = i;
            break;
        }
    }
    if (window_index >= 0 && window_index < gui_state.window_count - 1) {
        gui_window_t temp = gui_state.windows[window_index];
        for (int i = window_index; i < gui_state.window_count - 1; i++) {
            gui_state.windows[i] = gui_state.windows[i + 1];
        }
        gui_state.windows[gui_state.window_count - 1] = temp;
    }
    for (int i = 0; i < gui_state.window_count; i++) {
        gui_state.windows[i].focused = 0;
    }
    window->focused = 1;
    gui_state.focused_window_id = window->id;
    gui_needs_redraw = 1;
}
void gui_minimize_window(gui_window_t* window) {
    if (!window) return;
    if (window->state == WINDOW_STATE_MINIMIZED) {
        window->state = WINDOW_STATE_NORMAL;
        window->x = window->minimized_x;
        window->y = window->minimized_y;
    } else {
        window->minimized_x = window->x;
        window->minimized_y = window->y;
        window->state = WINDOW_STATE_MINIMIZED;
    }
    gui_needs_redraw = 1;
}
void gui_move_window(gui_window_t* window, int x, int y) {
    if (!window) return;
    int old_x = window->x;
    int old_y = window->y;
    int old_width = window->width;
    int old_height = window->height;
    u32 fb_w = get_fb_width();
    u32 fb_h = get_fb_height();
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + old_width > (int)fb_w) x = (int)fb_w - old_width;
    if (y + old_height > (int)fb_h) y = (int)fb_h - old_height;
    int old_min_x = old_x;
    int old_min_y = old_y;
    int old_max_x = old_x + old_width;
    int old_max_y = old_y + old_height;
    int new_min_x = x;
    int new_min_y = y;
    int new_max_x = x + old_width;
    int new_max_y = y + old_height;
    window->x = x;
    window->y = y;
    int dx = x - old_x;
    int dy = y - old_y;
   if ((dx >= -5 && dx <= 5) && (dy >= -5 && dy <= 5)) {
        if (dx > 0) {
            gui_mark_dirty(old_min_x, old_min_y, dx, old_height);
        } else if (dx < 0) {
            gui_mark_dirty(old_max_x + dx, old_min_y, -dx, old_height);
        }
        if (dy > 0) {
            gui_mark_dirty(new_min_x, old_min_y, old_width, dy);
        } else if (dy < 0) {
            gui_mark_dirty(new_min_x, old_max_y + dy, old_width, -dy);
        }
    } else {
        int min_x = (old_min_x < new_min_x) ? old_min_x : new_min_x;
        int min_y = (old_min_y < new_min_y) ? old_min_y : new_min_y;
        int max_x = (old_max_x > new_max_x) ? old_max_x : new_max_x;
        int max_y = (old_max_y > new_max_y) ? old_max_y : new_max_y;
        gui_mark_dirty(min_x, min_y, max_x - min_x, max_y - min_y);
    }
}
gui_button_t* gui_create_button(const char* text, int x, int y, int width, int height, void (*on_click)(gui_button_t*)) {
    if (gui_state.button_count >= 20) {
        return NULL;
    }
    gui_button_t* button = &gui_state.buttons[gui_state.button_count];
    str_copy(button->text, text);
    button->x = x;
    button->y = y;
    button->width = width;
    button->height = height;
    button->state = BUTTON_STATE_NORMAL;
    button->pressed = 0;
    button->on_click = on_click;
    button->user_data = NULL;
    gui_state.button_count++;
    gui_needs_redraw = 1;
    return button;
}
void gui_set_background_color(u32 color) {
    gui_state.background_color = color;
    if (!gui_state.wallpaper) {
        gui_needs_redraw = 1;
    }
}
int gui_load_wallpaper(const char* path) {
    bmp_image_t* image = gui_load_bmp(path);
    if (!image) {
        return 0;
    }
    if (gui_state.wallpaper) {
        gui_free_bmp(gui_state.wallpaper);
    }
    gui_state.wallpaper = image;
    gui_needs_redraw = 1;
    return 1;
}
int gui_load_cursor(const char* path) {
    bmp_image_t* image = gui_load_bmp(path);
    if (!image) {
        return 0;
    }
    if (gui_state.cursor_image) {
        gui_free_bmp(gui_state.cursor_image);
    }
    gui_state.cursor_image = image;
    gui_needs_redraw = 1;
    return 1;
}
void gui_set_cursor_visible(int visible) {
    gui_state.cursor_visible = visible;
    gui_needs_redraw = 1;
}
void gui_draw_wallpaper() {
    u32 fb_w = get_fb_width();
    u32 fb_h = get_fb_height();
    draw_rect(0, 0, (int)fb_w, (int)fb_h, DESKTOP_COLOR);
}
void gui_draw_taskbar() {
    u32 fb_w = get_fb_width();
    draw_rect(0, 0, (int)fb_w, 30, 0x80000000);
    int button_x = 100;
    for (int i = 0; i < gui_state.taskbar_button_count; i++) {
        taskbar_button_t* button = &gui_state.taskbar_buttons[i];
        str_copy(button->text, gui_state.windows[i].title);
        u32 button_color = 0x60000000;
        if (button->state == BUTTON_STATE_HOVER) {
            button_color = 0x80000000;
        } else if (button->state == BUTTON_STATE_PRESSED) {
            button_color = 0xA0000000;
        }
        draw_rect(button_x + 1, 6, TASKBAR_BUTTON_WIDTH - 2, 20, button_color);
        draw_rect(button_x, 5, TASKBAR_BUTTON_WIDTH, 1, 0x80000000);
        draw_rect(button_x, 5, 1, 20, 0x80000000);
        draw_rect(button_x + TASKBAR_BUTTON_WIDTH - 1, 5, 1, 20, 0x40000000);
        draw_rect(button_x, 5 + 20 - 1, TASKBAR_BUTTON_WIDTH, 1, 0x40000000);
        int text_x = button_x + 5;
        int text_y = 5 + 2;
        gui_draw_text_fast(button->text, text_x, text_y, 0xFFFFFFFF);
        button_x += TASKBAR_BUTTON_WIDTH + TASKBAR_BUTTON_SPACING;
    }
    cmos_time_t time;
    cmos_read_time(&time);
    char time_str[20];
    str_copy(time_str, "");
    if (time.hour < 10) str_append(time_str, "0");
    str_append_uint(time_str, time.hour);
    str_append(time_str, ":");
    if (time.minute < 10) str_append(time_str, "0");
    str_append_uint(time_str, time.minute);
    str_append(time_str, " ");
    if (time.month < 10) str_append(time_str, "0");
    str_append_uint(time_str, time.month);
    str_append(time_str, "/");
    if (time.day < 10) str_append(time_str, "0");
    str_append_uint(time_str, time.day);
    str_append(time_str, "/");
    str_append_uint(time_str, time.year % 100);
    int time_x = (int)fb_w - str_len(time_str) * 8 - 10;
    int time_y = 5 + 2;
    gui_draw_text_fast(time_str, time_x, time_y, 0xFFFFFFFF);
}
void gui_draw_window(gui_window_t* window) {
    if (!window || !window->visible || window->state == WINDOW_STATE_MINIMIZED) {
        return;
    }
    int radius = 12;
    draw_rounded_rect(window->x, window->y, window->width, window->height, radius, 0xFFFFFFFF);
    draw_rect(window->x, window->y, window->width, 1, 0xFF000000);
    draw_rect(window->x, window->y, 1, window->height, 0xFF000000);
    draw_rect(window->x + window->width - 1, window->y, 1, window->height, 0xFF000000);
    draw_rect(window->x, window->y + window->height - 1, window->width, 1, 0xFF000000);
    int content_x = window->x;
    int content_y = window->y;
    int content_w = window->width;
    int content_h = window->height;
    int bg_x, bg_y, bg_w, bg_h;
    if (str_equals(window->title, "Start Menu")) {
        bg_x = content_x + 1;
        bg_y = content_y + 1;
        bg_w = content_w - 2;
        bg_h = content_h - 2;
    } else {
        u32 title_color = window->focused ? 0xFF1A252F : 0xFF2C3E50;
        int title_x = content_x;
        int title_y = content_y;
        int title_w = content_w;
        int title_h = WINDOW_TITLE_HEIGHT;
        draw_rect(title_x, title_y, title_w, title_h, title_color);
        draw_rect(title_x, title_y + title_h, title_w, 1, 0xFF000000);
        bg_x = content_x;
        bg_y = content_y + WINDOW_TITLE_HEIGHT;
        bg_w = content_w;
        bg_h = content_h - WINDOW_TITLE_HEIGHT;
        int title_text_x = content_x + 15;
        int title_text_y = content_y + 8;
        gui_draw_text_fast(window->title, title_text_x, title_text_y, 0xFF333333);
        int close_x = content_x + content_w - 30;
        int close_y = content_y + 4;
        int close_size = 16;
        u32 close_color = 0xFFE74C3C;
        if (gui_is_point_in_rect(gui_state.cursor_x, gui_state.cursor_y, close_x, close_y, close_size, close_size)) {
            close_color = 0xFFC0392B;
        }
        draw_circle(close_x + close_size/2, close_y + close_size/2, close_size/2, close_color);
        int center_x = close_x + close_size / 2;
        int center_y = close_y + close_size / 2;
        draw_line(center_x - 3, center_y - 3, center_x + 3, center_y + 3, 0xFFFFFFFF);
        draw_line(center_x + 3, center_y - 3, center_x - 3, center_y + 3, 0xFFFFFFFF);
    }
    if (str_equals(window->title, "Terminal")) {
        draw_rounded_rect(bg_x, bg_y, bg_w, bg_h, 5, 0xFF1E1E1E);
        int text_x = bg_x + 12;
        int text_y = bg_y + 12;
        gui_terminal_draw(window, text_x, text_y);
        gui_needs_redraw = 1;
    } else if (str_equals(window->title, "Start Menu")) {
        gui_draw_start_menu(window);
    } else {
        gui_draw_text_fast(window->content, bg_x + 10, bg_y + 10, 0xFF333333);
    }
}
void gui_draw_button(gui_button_t* button) {
    if (!button) return;
    u32 button_color = 0xFFECF0F1;
    u32 text_color = 0xFF2C3E50;
    if (button->state == BUTTON_STATE_PRESSED) {
        button_color = 0xFFBDC3C7;
        text_color = 0xFF1A252F;
    } else if (button->state == BUTTON_STATE_HOVER) {
        button_color = 0xFFD5DBDB;
    }
    if (str_equals(button->text, "Start")) {
        draw_rect(button->x, button->y, button->width, button->height, button_color);
    } else {
        draw_rounded_rect(button->x, button->y, button->width, button->height, 6, button_color);
    }
    int text_x = button->x + (button->width - str_len(button->text) * 8) / 2;
    int text_y = button->y + (button->height - 16) / 2;
    gui_draw_text_fast(button->text, text_x, text_y, text_color);
}
int gui_is_point_in_rect(int px, int py, int rx, int ry, int rw, int rh) {
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}
gui_window_t* gui_get_window_at_point(int x, int y) {
    for (int i = gui_state.window_count - 1; i >= 0; i--) {
        gui_window_t* window = &gui_state.windows[i];
        if (window->visible && window->state != WINDOW_STATE_MINIMIZED &&
            gui_is_point_in_rect(x, y, window->x, window->y, window->width, window->height)) {
            return window;
        }
    }
    return NULL;
}
gui_button_t* gui_get_button_at_point(int x, int y) {
    for (int i = 0; i < gui_state.button_count; i++) {
        gui_button_t* button = &gui_state.buttons[i];
        if (gui_is_point_in_rect(x, y, button->x, button->y, button->width, button->height)) {
            return button;
        }
    }
    return NULL;
}
int gui_get_taskbar_button_at_point(int x, int y) {
    if (y > TASKBAR_HEIGHT) return -1;
    int button_x = 100;
    for (int i = 0; i < gui_state.taskbar_button_count; i++) {
        if (gui_is_point_in_rect(x, y, button_x, 5, TASKBAR_BUTTON_WIDTH, TASKBAR_BUTTON_HEIGHT)) {
            return i;
        }
        button_x += TASKBAR_BUTTON_WIDTH + TASKBAR_BUTTON_SPACING;
    }
    return -1;
}
void gui_handle_mouse_click(int x, int y, int button) {
    (void)button;
    gui_state.cursor_x = x;
    gui_state.cursor_y = y;
    int taskbar_button = gui_get_taskbar_button_at_point(x, y);
    if (taskbar_button >= 0) {
        gui_minimize_window(&gui_state.windows[taskbar_button]);
        gui_needs_redraw = 1;
        return;
    }
    gui_window_t* window = gui_get_window_at_point(x, y);
    if (window) {
        gui_focus_window(window);
        if (str_equals(window->title, "Start Menu")) {
            int menu_x = window->x;
            int menu_y = window->y;
            int menu_w = window->width;
            int current_y = menu_y + 10;
            int item_h = 30;
            int item_spacing = 2;
            int section_padding = 8;
            current_y += section_padding;
            if (gui_is_point_in_rect(x, y, menu_x + 20, current_y + 8, menu_w - 40, item_h)) {
                launch_terminal_new();
                gui_needs_redraw = 1;
                return;
            }
            current_y += item_h + item_spacing;
            if (gui_is_point_in_rect(x, y, menu_x + 20, current_y + 8, menu_w - 40, item_h)) {
                launch_shutdown_new();
                gui_needs_redraw = 1;
                return;
            }
            current_y += item_h + item_spacing;
            if (gui_is_point_in_rect(x, y, menu_x + 20, current_y + 8, menu_w - 40, item_h)) {
                launch_shutdown_new();
                gui_needs_redraw = 1;
                return;
            }
            current_y += item_h + item_spacing;
            current_y += section_padding;
            current_y += section_padding;
            if (gui_is_point_in_rect(x, y, menu_x + 20, current_y + 8, menu_w - 40, item_h)) {
                launch_shutdown_new();
                gui_needs_redraw = 1;
                return;
            }
            current_y += item_h + item_spacing;
            current_y += section_padding;
            current_y += section_padding;
            if (gui_is_point_in_rect(x, y, menu_x + 20, current_y + 8, menu_w - 40, item_h)) {
                launch_text_editor_new();
                gui_needs_redraw = 1;
                return;
            }
            current_y += item_h + item_spacing;
            if (gui_is_point_in_rect(x, y, menu_x + 20, current_y + 8, menu_w - 40, item_h)) {
                launch_shutdown_new();
                gui_needs_redraw = 1;
                return;
            }
            current_y += item_h + item_spacing;
            current_y += section_padding;
            current_y += section_padding;
            if (gui_is_point_in_rect(x, y, menu_x + 20, current_y + 8, menu_w - 40, item_h)) {
                launch_shutdown_new();
                gui_needs_redraw = 1;
                return;
            }
            gui_needs_redraw = 1;
            return;
        }
        if (!str_equals(window->title, "Start Menu")) {
            int close_x = window->x + window->width - 30;
            int close_y = window->y + 4;
            int close_size = 16;
            if (gui_is_point_in_rect(x, y, close_x, close_y, close_size, close_size)) {
                gui_destroy_window(window);
                return;
            }
            if (window->is_terminal) {
                extern void gui_terminal_handle_scrollbar_click(int x, int y);
                gui_terminal_handle_scrollbar_click(x, y);
            }
            int title_y_start = window->y + WINDOW_BORDER_WIDTH;
            int title_y_end = title_y_start + WINDOW_TITLE_HEIGHT;
            if (y >= title_y_start && y <= title_y_end) {
                gui_state.drag_state.dragging = 1;
                gui_state.drag_state.dragged_window = window;
                gui_state.drag_state.drag_start_x = x;
                gui_state.drag_state.drag_start_y = y;
                gui_state.drag_state.drag_offset_x = x - window->x;
                gui_state.drag_state.drag_offset_y = y - window->y;
                gui_needs_redraw = 1;
            }
        }
        gui_needs_redraw = 1;
        return;
    }
    gui_button_t* gui_button = gui_get_button_at_point(x, y);
    if (gui_button && gui_button->on_click) {
        gui_button->on_click(gui_button);
        gui_needs_redraw = 1;
        return;
    }
    gui_needs_redraw = 1;
}
void gui_handle_mouse_move(int x, int y) {
    gui_state.cursor_x = x;
    gui_state.cursor_y = y;
    extern void gui_terminal_handle_scrollbar_drag(int x, int y);
    gui_terminal_handle_scrollbar_drag(x, y);
if (gui_state.drag_state.dragging && gui_state.drag_state.dragged_window) {
    int new_x = x - gui_state.drag_state.drag_offset_x;
    int new_y = y - gui_state.drag_state.drag_offset_y;
    int old_x = gui_state.drag_state.dragged_window->x;
    int old_y = gui_state.drag_state.dragged_window->y;
    int old_width = gui_state.drag_state.dragged_window->width;
    int old_height = gui_state.drag_state.dragged_window->height;
    gui_state.drag_state.dragged_window->x = new_x;
    gui_state.drag_state.dragged_window->y = new_y;
    int min_x = (old_x < new_x) ? old_x : new_x;
    int min_y = (old_y < new_y) ? old_y : new_y;
    int max_x = (old_x + old_width > new_x + old_width) ? old_x + old_width : new_x + old_width;
    int max_y = (old_y + old_height > new_y + old_height) ? old_y + old_height : new_y + old_height;
    gui_mark_dirty(min_x, min_y, max_x - min_x, max_y - min_y);
    if (gui_state.drag_state.dragged_window->is_terminal) {
        gui_needs_redraw = 1;
    }
    mouse_cursor_needs_redraw = 1;
    return;
}
    int needs_redraw = 0;
    for (int i = 0; i < gui_state.button_count; i++) {
        gui_button_t* button = &gui_state.buttons[i];
        button_state_t old_state = button->state;
        if (gui_is_point_in_rect(x, y, button->x, button->y, button->width, button->height)) {
            button->state = BUTTON_STATE_HOVER;
        } else {
            button->state = BUTTON_STATE_NORMAL;
        }
        if (old_state != button->state) {
            needs_redraw = 1;
        }
    }
    int taskbar_button = gui_get_taskbar_button_at_point(x, y);
    for (int i = 0; i < gui_state.taskbar_button_count; i++) {
        taskbar_button_t* button = &gui_state.taskbar_buttons[i];
        button_state_t old_state = button->state;
        if (i == taskbar_button) {
            button->state = BUTTON_STATE_HOVER;
        } else {
            button->state = BUTTON_STATE_NORMAL;
        }
        if (old_state != button->state) {
            needs_redraw = 1;
        }
    }
    if (needs_redraw) {
        gui_needs_redraw = 1;
    }
    mouse_cursor_needs_redraw = 1;
}
void gui_handle_mouse_release(int x, int y, int button) {
    (void)button;
    gui_state.cursor_x = x;
    gui_state.cursor_y = y;
    extern void gui_terminal_handle_scrollbar_release();
    gui_terminal_handle_scrollbar_release();
    if (gui_state.drag_state.dragging) {
        gui_state.drag_state.dragging = 0;
        gui_state.drag_state.dragged_window = NULL;
    }
}
void gui_init() {
    u32 fb_w = get_fb_width();
    u32 fb_h = get_fb_height();
       if (fb_w == 0 || fb_h == 0 || fb_w > 10000 || fb_h > 10000) {
        char debug_buf[128];
 str_copy(debug_buf, "GUI: Invalid framebuffer size ");
 str_append_uint(debug_buf, fb_w);
 str_append(debug_buf, "x");
 str_append_uint(debug_buf, fb_h);
 str_append(debug_buf, "\n");
 printbs(debug_buf, GFX_RED);
        return;
    }
 int max_term_width = (int)fb_w - 100;
 int max_term_height = (int)fb_h - 150;
 if (max_term_width < 300) max_term_width = (int)fb_w > 300 ? 300 : (int)fb_w - 50;
 if (max_term_height < 200) max_term_height = (int)fb_h > 200 ? 200 : (int)fb_h - 50;
 if (max_term_width > (int)fb_w) max_term_width = (int)fb_w - 50;
 if (max_term_height > (int)fb_h) max_term_height = (int)fb_h - 50;
    gui_state.window_count = 0;
    gui_state.button_count = 0;
    gui_state.taskbar_button_count = 0;
    gui_state.focused_window_id = 0;
    gui_state.next_window_id = 1;
    gui_state.background_color = DESKTOP_COLOR;
    gui_state.wallpaper = NULL;
    gui_state.cursor_image = NULL;
    gui_state.cursor_x = 0;
    gui_state.cursor_y = 0;
    gui_state.cursor_visible = 1;
    gui_state.drag_state.dragging = 0;
    gui_state.drag_state.dragged_window = NULL;
    gui_state.cursor_visible = 1;
    gui_state.next_window_id = 1;
    graphics_enable_double_buffering();
    gui_running = 1;
    gui_mode = 1;
    char buf[64];
    str_copy(buf, "GUI_INIT: gui_running FORCED to 1\n");
    printbs(buf, GFX_GREEN);
    gui_needs_redraw = 1;
    mouse_cursor_needs_redraw = 1;
    gui_create_button("Start", 10, 5, 80, BUTTON_HEIGHT, on_start_button_click);
        gui_terminal_init();
        gui_load_wallpaper("/boot/wallpaper.bmp");
        gui_initialized = 1;
    char debug_buf[64];
    str_copy(debug_buf, "GUI: Before mouse setup\n");
    printbs(debug_buf, GFX_YELLOW);
    mouse_set_callback(gui_mouse_event_handler);
    usb_mouse_set_callback(gui_mouse_event_handler);
    extern int usb_keyboard_init(void);
    extern void usb_keyboard_set_callback(void (*cb)(int));
    usb_keyboard_init();
    usb_keyboard_set_callback(gui_handle_key);
    str_copy(debug_buf, "GUI: Before mouse position\n");
    printbs(debug_buf, GFX_YELLOW);
    int center_x = fb_w / 2;
    int center_y = fb_h / 2;
    gui_state.cursor_x = center_x;
    gui_state.cursor_y = center_y;
    str_copy(debug_buf, "GUI: Before timer register\n");
    printbs(debug_buf, GFX_YELLOW);
    timer_register_callback(gui_timer_callback);
    str_copy(debug_buf, "GUI: Init completed successfully\n");
    printbs(debug_buf, GFX_GREEN);
}
void gui_timer_callback(void) {
    if (gui_running) {
        gui_run();
    }
}
void gui_run() {
    if (!gui_running) return;
    static u64 last_frame_time = 0;
    u64 current_time = timer_get_ticks();
    if (current_time - last_frame_time < FRAME_TIME_MS) {
        return;
    }
    last_frame_time = current_time;
    gui_frame_count++;
    int has_changes = gui_needs_redraw || mouse_cursor_needs_redraw || (dirty_rect_count > 0);
    if (!has_changes) {
        return;
    }
    if (gui_needs_redraw) {
        gui_draw_wallpaper();
        gui_draw_taskbar();
        for (int i = 0; i < gui_state.window_count; i++) {
            gui_draw_window(&gui_state.windows[i]);
        }
        for (int i = 0; i < gui_state.button_count; i++) {
            gui_draw_button(&gui_state.buttons[i]);
        }
        gui_needs_redraw = 0;
    } else if (dirty_rect_count > 0) {
        for (int i = 0; i < dirty_rect_count; i++) {
            dirty_rect_t* rect = &dirty_rects[i];
            gui_redraw_region(rect->x, rect->y, rect->width, rect->height);
        }
        dirty_rect_count = 0;
    }
    extern void mouse_draw_cursor();
    mouse_draw_cursor();
    if (graphics_is_double_buffering_enabled()) {
        graphics_swap_buffers();
        static int swap_counter = 0;
        if (++swap_counter >= 60) {
            swap_counter = 0;
            char buf[64];
            str_copy(buf, "GUI: Frame rendered, FPS: ");
            str_append_uint(buf, 1000 / FRAME_TIME_MS);
            str_append(buf, "\n");
            printbs(buf, GFX_GREEN);
        }
    }
    gui_needs_redraw = 0;
    mouse_cursor_needs_redraw = 0;
}
void gui_draw_start_menu_old(gui_window_t* window) {
    if (!window || !str_equals(window->title, "Start Menu")) {
        return;
    }
    draw_rect(window->x, window->y, window->width, window->height, 0xFF202020);
    draw_rect(window->x + 1, window->y + 1, window->width - 2, window->height - 2, 0xFF303030);
}
static void on_start_button_click(gui_button_t* button) {
    (void)button;
    for (int i = 0; i < gui_state.window_count; i++) {
        if (str_equals(gui_state.windows[i].title, "Start Menu")) {
            gui_destroy_window(&gui_state.windows[i]);
            gui_needs_redraw = 1;
            return;
        }
    }
    int start_menu_y = TASKBAR_HEIGHT + 10;
    gui_window_t* window = gui_create_window("Start Menu", 10, start_menu_y, 220, 380);
    if (window) {
        gui_needs_redraw = 1;
    }
}
void gui_handle_key(int key) {
    for (int i = 0; i < gui_state.window_count; i++) {
        if (str_equals(gui_state.windows[i].title, "Terminal")) {
            gui_focus_window(&gui_state.windows[i]);
            break;
        }
    }
    gui_terminal_handle_key(key);
    gui_needs_redraw = 1;
}
void gui_set_needs_redraw(int needs_redraw) {
    gui_needs_redraw = needs_redraw;
}
void gui_mouse_event_handler(int32_t x, int32_t y, uint8_t buttons) {
    if (usb_mouse_is_initialized()) {
        usb_mouse_set_position(x, y);
    } else if (mouse_is_initialized()) {
        mouse_set_position(x, y);
    }
    gui_state.cursor_x = x;
    gui_state.cursor_y = y;
    static uint8_t last_buttons = 0;
    uint8_t changed_buttons = buttons ^ last_buttons;
    if (changed_buttons & 0x01) {
        if (buttons & 0x01) {
            gui_handle_mouse_click(x, y, 0);
        } else {
            gui_handle_mouse_release(x, y, 0);
        }
    }
    gui_handle_mouse_move(x, y);
    if (buttons & 0x08) {
        gui_terminal_handle_scroll(1);
    } else if (buttons & 0x10) {
        gui_terminal_handle_scroll(-1);
    }
    gui_needs_redraw = 1;
    last_buttons = buttons;
    mouse_cursor_needs_redraw = 1;
}