#include <kernel/gui/gui.h>
#include <string/string.h>
#include <kernel/graph/graphics.h>
#include <drivers/ps2/mouse/mouse.h>
#include <drivers/usb/usb_mouse.h>
#include <kernel/console/graph/dos.h>
#include <kernel/console/console.h>
#include <kernel/exceptions/timer.h>
#include <kernel/file_systems/vfs/vfs.h>
#include <kernel/mem/klime/klime.h>
static void gui_terminal_print(const char *text, u32 color);
gui_state_t gui_state = {0};
int gui_running = 0;
int gui_mode = 0;
static char gui_input_buffer[256];
static int gui_input_pos = 0;
static int gui_cursor_visible = 1;
static int gui_key_pressed = 0;
static int gui_initialized = 0;
static int gui_needs_redraw = 1;
static int mouse_cursor_needs_redraw = 0;
static int gui_show_prompt = 1;
static int prev_mouse_x = -1;
static int prev_mouse_y = -1;
static int gui_frame_count = 0;
#define GUI_OUTPUT_LINES 20
#define GUI_OUTPUT_COLS 80
static char gui_output_buffer[GUI_OUTPUT_LINES][GUI_OUTPUT_COLS];
static int gui_output_line = 0;
static int gui_frame_counter = 0;
static u64 last_frame_time = 0;
#define FRAME_TIME_MS 16
static dirty_rect_t dirty_rects[MAX_DIRTY_RECTS];
static int dirty_rect_count = 0;
extern void* fs_klime;
static void on_wallpaper_button_click(gui_button_t* button);
static void on_settings_button_click(gui_button_t* button);
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
    if (read_bytes != sizeof(bmp_file_header_t) || file_header.type != 0x4D42) {
        print("Invalid BMP file header\n", GFX_RED);
        fs_close(fd);
        return NULL;
    }
    read_bytes = fs_read(fd, &info_header, sizeof(bmp_info_header_t));
    if (read_bytes != sizeof(bmp_info_header_t)) {
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
    read_bytes = fs_read(fd, bmp_pixels, info_header.width * info_header.height * 4);
    fs_close(fd);
    if (read_bytes != (ssize_t)(info_header.width * info_header.height * 4)) {
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
void gui_terminal_clear() {
    for (int i = 0; i < GUI_OUTPUT_LINES; i++) {
        gui_output_buffer[i][0] = '\0';
    }
    gui_output_line = 0;
    gui_input_pos = 0;
    gui_input_buffer[0] = '\0';
    gui_terminal_print("user@host:/$ ", GFX_BLACK);
    gui_needs_redraw = 1;
}
void gui_reset_input_line() {
    gui_input_pos = 0;
    gui_input_buffer[0] = '\0';
    gui_terminal_print("user@host:/$ ", GFX_BLACK);
}
void gui_mark_dirty(int x, int y, int width, int height) {
    if (dirty_rect_count >= MAX_DIRTY_RECTS) {
        gui_needs_redraw = 1;
        return;
    }
    for (int i = 0; i < dirty_rect_count; i++) {
        dirty_rect_t* rect = &dirty_rects[i];
        if (rect->x <= x + width && rect->x + rect->width >= x &&
            rect->y <= y + height && rect->y + rect->height >= y) {
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
    int region_left = x;
    int region_right = x + width;
    int region_top = y;
    int region_bottom = y + height;
    for (int i = 0; i < gui_state.window_count; i++) {
        gui_window_t* window = &gui_state.windows[i];
        if (window->visible && window->state != WINDOW_STATE_MINIMIZED) {
            int win_left = window->x;
            int win_right = window->x + window->width;
            int win_top = window->y;
            int win_bottom = window->y + window->height;
            int region_left = x;
            int region_right = x + width;
            int region_top = y;
            int region_bottom = y + height;
            if (win_left < region_right && win_right > region_left &&
                win_top < region_bottom && win_bottom > region_top) {
                gui_draw_window(window);
                break;
            }
        }
    }
    for (int i = 0; i < gui_state.button_count; i++) {
        gui_button_t* button = &gui_state.buttons[i];
        int btn_left = button->x;
        int btn_right = button->x + button->width;
        int btn_top = button->y;
        int btn_bottom = button->y + button->height;
        if (btn_left < region_right && btn_right > region_left &&
            btn_top < region_bottom && btn_bottom > region_top) {
            gui_draw_button(button);
        }
    }
}
void gui_terminal_print(const char *text, u32 color) {
    const char *p = text;
    while (*p) {
        if (*p == '\n') {
            gui_output_line++;
            if (gui_output_line >= GUI_OUTPUT_LINES) {
                for (int i = 1; i < GUI_OUTPUT_LINES; i++) {
                    str_copy(gui_output_buffer[i-1], gui_output_buffer[i]);
                }
                str_copy(gui_output_buffer[GUI_OUTPUT_LINES-1], "");
                gui_output_line = GUI_OUTPUT_LINES - 1;
            }
        } else {
            int line = gui_output_line;
            int col = str_len(gui_output_buffer[line]);
            if (col < GUI_OUTPUT_COLS - 1) {
                gui_output_buffer[line][col] = *p;
                gui_output_buffer[line][col + 1] = '\0';
            }
        }
        p++;
    }
    gui_needs_redraw = 1;
}
void gui_draw_text_fast(const char* str, int x, int y, u32 color) {
    int current_x = x;
    const u8* glyph;
    u32 glyph_width = fm_get_char_width();
    u32 glyph_height = fm_get_char_height();
    for (size_t i = 0; str[i]; i++) {
        char c = str[i];
        if (c == '\n') {
            current_x = x;
            y += glyph_height + 2;
            continue;
        }
        glyph = fm_get_glyph((u8)c);
        if (!glyph) {
            current_x += glyph_width;
            continue;
        }
        for (u32 dy = 0; dy < glyph_height; dy++) {
            u8 row = glyph[dy];
            for (u32 dx = 0; dx < glyph_width; dx++) {
                if (row & (1 << (7 - dx))) {
                    int screen_x = current_x + dx;
                    int screen_y = y + dy;
                    if (screen_x >= 0 && screen_x < get_fb_width() &&
                        screen_y >= 0 && screen_y < get_fb_height()) {
                        putpixel(screen_x, screen_y, color);
                    }
                }
            }
        }
        current_x += glyph_width;
    }
}
gui_window_t* gui_create_window(const char* title, int x, int y, int width, int height) {
    if (gui_state.window_count >= MAX_WINDOWS) {
        return NULL;
    }
    gui_window_t* window = &gui_state.windows[gui_state.window_count];
    window->id = gui_state.next_window_id++;
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    str_copy(window->title, title);
    window->focused = 1;
    window->state = WINDOW_STATE_NORMAL;
    window->visible = 1;
    window->content[0] = '\0';
    for (int i = 0; i < gui_state.window_count; i++) {
        gui_state.windows[i].focused = 0;
    }
    gui_state.focused_window_id = window->id;
    gui_state.window_count++;
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
    window->x = x;
    window->y = y;
    gui_needs_redraw = 1;
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
    draw_rect(0, 0, fb_w, fb_h, DESKTOP_COLOR);
}
void gui_draw_taskbar() {
    u32 fb_w = get_fb_width();
    u32 fb_h = get_fb_height();
    draw_rect(0, fb_h - TASKBAR_HEIGHT, fb_w, TASKBAR_HEIGHT, TASKBAR_COLOR);
    draw_rect(0, fb_h - TASKBAR_HEIGHT, fb_w, 1, BUTTON_BORDER_LIGHT);
    draw_rect(0, fb_h - 1, fb_w, 1, BUTTON_BORDER_DARK);
    int button_x = 5;
    for (int i = 0; i < gui_state.taskbar_button_count; i++) {
        taskbar_button_t* button = &gui_state.taskbar_buttons[i];
        str_copy(button->text, gui_state.windows[i].title);
        u32 button_color = TASKBAR_BUTTON_COLOR;
        if (button->state == BUTTON_STATE_HOVER) {
            button_color = TASKBAR_BUTTON_HOVER_COLOR;
        } else if (button->state == BUTTON_STATE_PRESSED) {
            button_color = TASKBAR_BUTTON_ACTIVE_COLOR;
        }
        draw_rect(button_x + 1, fb_h - TASKBAR_HEIGHT + 6, TASKBAR_BUTTON_WIDTH - 2, TASKBAR_BUTTON_HEIGHT - 2, button_color);
        draw_rect(button_x, fb_h - TASKBAR_HEIGHT + 5, TASKBAR_BUTTON_WIDTH, 1, BUTTON_BORDER_LIGHT);
        draw_rect(button_x, fb_h - TASKBAR_HEIGHT + 5, 1, TASKBAR_BUTTON_HEIGHT, BUTTON_BORDER_LIGHT);
        draw_rect(button_x + TASKBAR_BUTTON_WIDTH - 1, fb_h - TASKBAR_HEIGHT + 5, 1, TASKBAR_BUTTON_HEIGHT, BUTTON_BORDER_DARK);
        draw_rect(button_x, fb_h - TASKBAR_HEIGHT + 5 + TASKBAR_BUTTON_HEIGHT - 1, TASKBAR_BUTTON_WIDTH, 1, BUTTON_BORDER_DARK);
        int text_x = button_x + 5;
        int text_y = fb_h - TASKBAR_HEIGHT + 5 + 5;
        gui_draw_text_fast(button->text, text_x, text_y, 0xFF000000);
        button_x += TASKBAR_BUTTON_WIDTH + TASKBAR_BUTTON_SPACING;
    }
}
void gui_draw_window(gui_window_t* window) {
    if (!window || !window->visible || window->state == WINDOW_STATE_MINIMIZED) {
        return;
    }
    draw_rect(window->x, window->y, window->width, window->height, 0xFF000000);
    draw_rect(window->x + 1, window->y + 1, window->width - 2, window->height - 2, 0xFFFFFFFF);
    draw_rect(window->x + 2, window->y + 2, window->width - 4, window->height - 4, 0xFF808080);
    int content_x = window->x + WINDOW_BORDER_WIDTH;
    int content_y = window->y + WINDOW_BORDER_WIDTH;
    int content_w = window->width - 2 * WINDOW_BORDER_WIDTH;
    int content_h = window->height - 2 * WINDOW_BORDER_WIDTH;
    draw_rect(content_x, content_y, content_w, content_h, 0xFF404040);
    u32 title_color = window->focused ? WINDOW_TITLE_COLOR_FOCUSED : WINDOW_TITLE_COLOR_UNFOCUSED;
    int title_x = content_x + 1;
    int title_y = content_y + 1;
    int title_w = content_w - 2;
    int title_h = WINDOW_TITLE_HEIGHT;
    draw_rect(title_x, title_y, title_w, title_h, title_color);
    if (window->focused) {
        for (int i = 0; i < 3; i++) {
            draw_rect(title_x, title_y + title_h - 1 - i, title_w, 1,
                     (title_color & 0xFFFFFF00) | ((title_color & 0xFF) - i * 20));
        }
    }
    draw_rect(title_x, title_y + title_h, title_w, 1, 0xFF000000);
    int bg_x = content_x + 1;
    int bg_y = content_y + WINDOW_TITLE_HEIGHT + 1;
    int bg_w = content_w - 2;
    int bg_h = content_h - WINDOW_TITLE_HEIGHT - 2;
    draw_rect(bg_x, bg_y, bg_w, bg_h, WINDOW_BG_COLOR);
    int title_text_x = content_x + 8;
    int title_text_y = content_y + 8;
    gui_draw_text_fast(window->title, title_text_x, title_text_y, WINDOW_TITLE_TEXT_COLOR);
    int close_x = content_x + content_w - 18;
    int close_y = content_y + 4;
    draw_rect(close_x, close_y, 15, 11, CLOSE_BUTTON_COLOR);
    draw_rect(close_x, close_y, 15, 1, 0xFFFFFFFF);
    draw_rect(close_x, close_y, 1, 11, 0xFFFFFFFF);
    draw_rect(close_x + 14, close_y, 1, 11, 0xFF404040);
    draw_rect(close_x, close_y + 10, 15, 1, 0xFF404040);
    int center_x = close_x + 7;
    int center_y = close_y + 5;
    draw_line(center_x - 3, center_y - 3, center_x + 3, center_y + 3, CLOSE_BUTTON_X_COLOR);
    draw_line(center_x + 3, center_y - 3, center_x - 3, center_y + 3, CLOSE_BUTTON_X_COLOR);
    int window_content_x = bg_x + 10;
    int window_content_y = bg_y + 10;
    if (str_equals(window->title, "Terminal")) {
        draw_rect(bg_x, bg_y, bg_w, bg_h, 0xFF000000);
        int text_x = bg_x + 10;
        int text_y = bg_y + 10;
        int line_y = text_y;
        int lines_shown = 0;
        for (int i = 0; i < GUI_OUTPUT_LINES - 1 && lines_shown < GUI_OUTPUT_LINES - 1; i++) {
            if (gui_output_buffer[i][0] != '\0') {
                if (str_starts_with(gui_output_buffer[i], "user@host:/$ ")) {
                    int prompt_len = str_len("user@host:/$ ");
                    gui_draw_text_line_fast("user@host:/$ ", text_x, line_y, 0xFF00FF00);
                    gui_draw_text_line_fast(gui_output_buffer[i] + prompt_len, text_x + (prompt_len * 8), line_y, 0xFFFFFFFF);
                } else {
                    gui_draw_text_line_fast(gui_output_buffer[i], text_x, line_y, 0xFFFFFFFF);
                }
                line_y += 16;
                lines_shown++;
            }
        }
        char current_line[GUI_OUTPUT_COLS];
        str_copy(current_line, "user@host:/$ ");
        str_append(current_line, gui_input_buffer);
        gui_draw_text_line_fast("user@host:/$ ", text_x, line_y, 0xFF00FF00);
        gui_draw_text_line_fast(gui_input_buffer, text_x + (13 * 8), line_y, 0xFFFFFFFF);
    } else {
        gui_draw_text_fast(window->content, window_content_x, window_content_y, 0xFF000000);
    }
}
void gui_draw_button(gui_button_t* button) {
    if (!button) return;
    u32 button_color = BUTTON_COLOR;
    u32 border_light = BUTTON_BORDER_LIGHT;
    u32 border_dark = BUTTON_BORDER_DARK;
    if (button->state == BUTTON_STATE_PRESSED) {
        button_color = BUTTON_ACTIVE_COLOR;
        border_light = BUTTON_BORDER_DARK;
        border_dark = BUTTON_BORDER_LIGHT;
    } else if (button->state == BUTTON_STATE_HOVER) {
        button_color = BUTTON_HOVER_COLOR;
    }
    draw_rect(button->x + 1, button->y + 1, button->width - 2, button->height - 2, button_color);
    draw_rect(button->x, button->y, button->width, 1, border_light);
    draw_rect(button->x, button->y, 1, button->height, border_light);
    draw_rect(button->x + button->width - 1, button->y, 1, button->height, border_dark);
    draw_rect(button->x, button->y + button->height - 1, button->width, 1, border_dark);
    int text_x = button->x + (button->width - str_len(button->text) * 8) / 2;
    int text_y = button->y + (button->height - 16) / 2;
    gui_draw_text_fast(button->text, text_x, text_y, 0xFF000000);
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
    u32 fb_h = get_fb_height();
    if (y < fb_h - TASKBAR_HEIGHT) return -1;
    int button_x = 5;
    for (int i = 0; i < gui_state.taskbar_button_count; i++) {
        if (gui_is_point_in_rect(x, y, button_x, fb_h - TASKBAR_HEIGHT + 5, TASKBAR_BUTTON_WIDTH, TASKBAR_BUTTON_HEIGHT)) {
            return i;
        }
        button_x += TASKBAR_BUTTON_WIDTH + TASKBAR_BUTTON_SPACING;
    }
    return -1;
}
void gui_handle_mouse_click(int x, int y, int button) {
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
        int close_x = window->x + window->width - WINDOW_BORDER_WIDTH - 20;
        int close_y = window->y + WINDOW_BORDER_WIDTH + 5;
        if (gui_is_point_in_rect(x, y, close_x, close_y, 15, 15)) {
            gui_destroy_window(window);
            return;
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
    if (gui_state.drag_state.dragging && gui_state.drag_state.dragged_window) {
        int new_x = x - gui_state.drag_state.drag_offset_x;
        int new_y = y - gui_state.drag_state.drag_offset_y;
        gui_move_window(gui_state.drag_state.dragged_window, new_x, new_y);
        return;
    }
    for (int i = 0; i < gui_state.button_count; i++) {
        gui_button_t* button = &gui_state.buttons[i];
        button_state_t old_state = button->state;
        if (gui_is_point_in_rect(x, y, button->x, button->y, button->width, button->height)) {
            button->state = BUTTON_STATE_HOVER;
        } else {
            button->state = BUTTON_STATE_NORMAL;
        }
        if (old_state != button->state) {
            gui_needs_redraw = 1;
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
            gui_needs_redraw = 1;
        }
    }
}
void gui_handle_mouse_release(int x, int y, int button) {
    gui_state.cursor_x = x;
    gui_state.cursor_y = y;
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
int max_term_width = fb_w - 100;
int max_term_height = fb_h - 150;
if (max_term_width < 300) max_term_width = fb_w > 300 ? 300 : fb_w - 50;
if (max_term_height < 200) max_term_height = fb_h > 200 ? 200 : fb_h - 50;
if (max_term_width > fb_w) max_term_width = fb_w - 50;
if (max_term_height > fb_h) max_term_height = fb_h - 50;
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
    char buf[64];
    str_copy(buf, "GUI_INIT: gui_running FORCED to 1\n");
    printbs(buf, GFX_GREEN);
    gui_window_t* terminal_window = gui_create_window("Terminal", 50, 50, max_term_width, max_term_height);
    if (terminal_window) {
        str_copy(terminal_window->content, "DystopiaOS GUI Terminal\nType 'exit' to return to console\n");
        if (gui_state.taskbar_button_count < MAX_WINDOWS) {
            gui_state.taskbar_buttons[gui_state.taskbar_button_count].window_id = terminal_window->id;
            str_copy(gui_state.taskbar_buttons[gui_state.taskbar_button_count].text, terminal_window->title);
            gui_state.taskbar_button_count++;
        }
    }
    gui_window_t* demo_window = gui_create_window("Demo Window", 100, 100, 300, 200);
    if (demo_window) {
        str_copy(demo_window->content, "This is a demo window!\nYou can move it around by dragging\nthe title bar.\n\nClick the X to close.");
        if (gui_state.taskbar_button_count < MAX_WINDOWS) {
            gui_state.taskbar_buttons[gui_state.taskbar_button_count].window_id = demo_window->id;
            str_copy(gui_state.taskbar_buttons[gui_state.taskbar_button_count].text, demo_window->title);
            gui_state.taskbar_button_count++;
        }
    }
    gui_needs_redraw = 1;
    mouse_cursor_needs_redraw = 1;
    gui_create_button("Load Wallpaper", fb_w - 150, fb_h - TASKBAR_HEIGHT + 5, 120, BUTTON_HEIGHT, on_wallpaper_button_click);
    gui_create_button("Settings", fb_w - 280, fb_h - TASKBAR_HEIGHT + 5, 120, BUTTON_HEIGHT, on_settings_button_click);
        gui_terminal_print("DystopiaOS GUI Terminal\n", GFX_BLACK);
        gui_terminal_print("Type 'exit' to return to console\n\n", GFX_BLACK);
        gui_terminal_print("user@host:/$ ", GFX_BLACK);
        gui_initialized = 1;
    char debug_buf[64];
    str_copy(debug_buf, "GUI: Before mouse setup\n");
    printbs(debug_buf, GFX_YELLOW);
    mouse_set_callback(gui_mouse_event_handler);
    usb_mouse_set_callback(gui_mouse_event_handler);
    extern int usb_keyboard_init(void);
    extern void usb_keyboard_set_callback(void (*cb)(char));
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
    gui_frame_count++;
    static int frame_counter = 0;
    frame_counter++;
    if (gui_key_pressed) {
        char key = gui_key_pressed;
        gui_key_pressed = 0;
        if (key == '\n') {
            gui_input_buffer[gui_input_pos] = '\0';
            gui_terminal_print("user@host:/$ ", GFX_BLACK);
            gui_terminal_print(gui_input_buffer, GFX_BLACK);
            gui_terminal_print("\n", GFX_BLACK);
            if (str_equals(gui_input_buffer, "exit")) {
                gui_running = 0;
                gui_initialized = 0;
                gui_mode = 0;
                graphics_disable_double_buffering();
                timer_unregister_callback(gui_timer_callback);
                mouse_unregister_callback();
                usb_mouse_unregister_callback();
                clear(CONSOLESCREEN_BG_COLOR);
                console_window_init();
                extern void shell_print_prompt();
                shell_print_prompt();
                extern void cursor_draw();
                extern void cursor_reset_blink();
                cursor_reset_blink();
                cursor_draw();
                return;
            }
            gui_mode = 1;
            console_execute(gui_input_buffer);
            gui_mode = 0;
            gui_reset_input_line();
        } else if (key == '\b') {
            if (gui_input_pos > 0) {
                gui_input_pos--;
                gui_input_buffer[gui_input_pos] = '\0';
                gui_needs_redraw = 1;
            }
        } else if (key >= 32 && key <= 126 && gui_input_pos < sizeof(gui_input_buffer) - 1) {
            gui_input_buffer[gui_input_pos++] = key;
            gui_input_buffer[gui_input_pos] = '\0';
            gui_needs_redraw = 1;
        }
    }
    if (!gui_needs_redraw && !mouse_cursor_needs_redraw && dirty_rect_count == 0) {
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
    if (0) {
        gui_window_t* terminal = &gui_state.windows[0];
        int text_x = terminal->x + WINDOW_BORDER_WIDTH + 10;
        int text_y = terminal->y + WINDOW_BORDER_WIDTH + WINDOW_TITLE_HEIGHT + 10;
        int term_width = terminal->width - 2 * WINDOW_BORDER_WIDTH - 20;
        int term_height = terminal->height - WINDOW_TITLE_HEIGHT - 2 * WINDOW_BORDER_WIDTH - 60;
        int line_y = text_y;
        for (int i = 0; i < GUI_OUTPUT_LINES; i++) {
            if (gui_output_buffer[i][0] != '\0') {
                gui_draw_text_line_fast(gui_output_buffer[i], text_x, line_y, 0xFF000000);
                line_y += 16;
            }
        }
        if (gui_show_prompt) {
            int prompt_y = text_y + (GUI_OUTPUT_LINES * 16);
            gui_draw_text_line_fast("user@host:/$ ", text_x, prompt_y, 0xFF000000);
            gui_draw_text_line_fast(gui_input_buffer, text_x + (13 * 8), prompt_y, 0xFF000000);
            int cursor_x_pos = text_x + (13 + str_len(gui_input_buffer)) * 8;
            for (int y = 0; y < 16; y++) {
                for (int x = 0; x < 2; x++) {
                    putpixel(cursor_x_pos + x, prompt_y + y, 0xFF000000);
                }
            }
        }
    }
    gui_needs_redraw = 0;
    mouse_cursor_needs_redraw = 0;
    extern void mouse_draw_cursor();
    mouse_draw_cursor();
    if (graphics_is_double_buffering_enabled()) {
        graphics_swap_buffers();
        static int swap_counter = 0;
        if (swap_counter++ % 20 == 0) {
            char buf[32];
            str_copy(buf, "GUI: buffers swapped\n");
            printbs(buf, GFX_GREEN);
        }
    }
}
static void on_wallpaper_button_click(gui_button_t* button) {
    gui_load_wallpaper("/boot/ui/assets/logo.bin");
}
static void on_settings_button_click(gui_button_t* button) {
    gui_window_t* settings_window = gui_create_window("Settings", 200, 150, 400, 300);
    if (settings_window) {
        str_copy(settings_window->content, "Settings Panel\n\nBackground Color: Click to change\nWallpaper: Load image\nCursor: Custom cursor loaded");
        if (gui_state.taskbar_button_count < MAX_WINDOWS) {
            gui_state.taskbar_buttons[gui_state.taskbar_button_count].window_id = settings_window->id;
            str_copy(gui_state.taskbar_buttons[gui_state.taskbar_button_count].text, settings_window->title);
            gui_state.taskbar_button_count++;
        }
    }
}
void gui_handle_key(char key) {
    if (key == '\b') {
        if (gui_input_pos > 0) {
            gui_input_pos--;
            gui_input_buffer[gui_input_pos] = '\0';
            gui_needs_redraw = 1;
        }
    } else if (key == '\n') {
        gui_input_buffer[gui_input_pos] = '\0';
        gui_mode = 1;
        console_execute(gui_input_buffer);
        gui_mode = 0;
        if (str_equals(gui_input_buffer, "exit")) {
            gui_running = 0;
            gui_initialized = 0;
            gui_mode = 0;
            graphics_disable_double_buffering();
            timer_unregister_callback(gui_timer_callback);
            mouse_unregister_callback();
            usb_mouse_unregister_callback();
            clear(CONSOLESCREEN_BG_COLOR);
            console_window_init();
            shell_print_prompt();
            cursor_reset_blink();
            cursor_draw();
            return;
        }
        gui_input_pos = 0;
        gui_input_buffer[0] = '\0';
        gui_needs_redraw = 1;
    } else if (key >= 32 && key <= 126 && gui_input_pos < sizeof(gui_input_buffer) - 1) {
        gui_input_buffer[gui_input_pos++] = key;
        gui_input_buffer[gui_input_pos] = '\0';
        gui_needs_redraw = 1;
    }
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
    last_buttons = buttons;
    gui_needs_redraw = 1;
}