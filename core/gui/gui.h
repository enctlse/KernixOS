#ifndef GUI_H
#define GUI_H
#include <outputs/types.h>
#define MAX_WINDOWS 10
#define WINDOW_TITLE_HEIGHT 25
#define WINDOW_BORDER_WIDTH 0
#define WINDOW_MIN_WIDTH 100
#define WINDOW_MIN_HEIGHT 50
#define TASKBAR_HEIGHT 35
#define TASKBAR_BUTTON_WIDTH 120
#define TASKBAR_BUTTON_HEIGHT 25
#define TASKBAR_BUTTON_SPACING 5
#define BUTTON_HEIGHT 25
#define BUTTON_PADDING 10
#define WINDOW_title_color_FOCUSED 0xFF000080
#define WINDOW_title_color_UNFOCUSED 0xFF808080
#define WINDOW_TITLE_text_color 0xFFFFFFFF
#define WINDOW_BORDER_COLOR_OUTER 0xFFD3D3D3
#define WINDOW_BORDER_COLOR_INNER 0xFFFFFFFF
#define WINDOW_BG_COLOR 0xFFC0C0C0
#define DESKTOP_COLOR 0xFF3F8077
#define TASKBAR_COLOR 0xFFC0C0C0
#define TASKBAR_BUTTON_COLOR 0xFFC0C0C0
#define TASKBAR_BUTTON_HOVER_COLOR 0xFFDFDFDF
#define TASKBAR_BUTTON_ACTIVE_COLOR 0xFFA0A0A0
#define BUTTON_COLOR 0xFFC0C0C0
#define BUTTON_HOVER_COLOR 0xFFDFDFDF
#define BUTTON_ACTIVE_COLOR 0xFF808080
#define BUTTON_BORDER_LIGHT 0xFFFFFFFF
#define BUTTON_BORDER_DARK 0xFF808080
#define CLOSE_BUTTON_COLOR 0xFFC0C0C0
#define CLOSE_BUTTON_X_COLOR 0xFF000000
typedef enum {
    WINDOW_STATE_NORMAL,
    WINDOW_STATE_MINIMIZED,
    WINDOW_STATE_MAXIMIZED
} window_state_t;
#define MAX_DIRTY_RECTS 32
typedef struct {
    int x, y, width, height;
} dirty_rect_t;
typedef enum {
    BUTTON_STATE_NORMAL,
    BUTTON_STATE_HOVER,
    BUTTON_STATE_PRESSED
} button_state_t;
typedef struct gui_window {
    int id;
    int x, y, width, height;
    char title[64];
    int focused;
    window_state_t state;
    int visible;
    int minimized_x, minimized_y;
    char content[1024];
    void (*on_close)(struct gui_window* window);
    void (*on_minimize)(struct gui_window* window);
    void (*on_maximize)(struct gui_window* window);
    int is_terminal;
} gui_window_t;
typedef struct gui_button {
    int x, y, width, height;
    char text[32];
    button_state_t state;
    int pressed;
    void (*on_click)(struct gui_button* button);
    void* user_data;
} gui_button_t;
typedef struct taskbar_button {
    int window_id;
    char text[32];
    button_state_t state;
    int pressed;
} taskbar_button_t;
typedef struct {
    int width;
    int height;
    u32* pixels;
} bmp_image_t;
typedef struct {
    int dragging;
    int drag_start_x, drag_start_y;
    int drag_offset_x, drag_offset_y;
    gui_window_t* dragged_window;
} drag_state_t;
typedef struct {
    int active_category;
    gui_window_t windows[MAX_WINDOWS];
    int window_count;
    int focused_window_id;
    taskbar_button_t taskbar_buttons[MAX_WINDOWS];
    int taskbar_button_count;
    gui_button_t buttons[20];
    int button_count;
    drag_state_t drag_state;
    bmp_image_t* wallpaper;
    u32 background_color;
    bmp_image_t* cursor_image;
    int cursor_x, cursor_y;
    int cursor_visible;
    int next_window_id;
} gui_state_t;
extern gui_state_t gui_state;
extern int gui_running;
void gui_set_needs_redraw(int needs_redraw);
void gui_init();
void gui_run();
void gui_timer_callback(void);
void gui_handle_key(int key);
void gui_mouse_event_handler(int32_t x, int32_t y, uint8_t buttons);
void gui_terminal_handle_scroll(int delta);
void gui_draw_start_menu(gui_window_t* window);
void gui_terminal_handle_scroll(int delta);
gui_window_t* gui_create_window(const char* title, int x, int y, int width, int height);
void gui_destroy_window(gui_window_t* window);
void gui_focus_window(gui_window_t* window);
void gui_minimize_window(gui_window_t* window);
void gui_maximize_window(gui_window_t* window);
void gui_move_window(gui_window_t* window, int x, int y);
void gui_resize_window(gui_window_t* window, int width, int height);
gui_button_t* gui_create_button(const char* text, int x, int y, int width, int height, void (*on_click)(gui_button_t*));
void gui_destroy_button(gui_button_t* button);
void gui_set_background_color(u32 color);
int gui_load_wallpaper(const char* path);
void gui_set_wallpaper(bmp_image_t* image);
int gui_load_cursor(const char* path);
void gui_set_cursor_visible(int visible);
bmp_image_t* gui_load_bmp(const char* path);
void gui_free_bmp(bmp_image_t* image);
void gui_draw_wallpaper();
void gui_draw_taskbar();
void gui_draw_window(gui_window_t* window);
void gui_draw_button(gui_button_t* button);
void gui_draw_cursor();
void gui_handle_mouse_click(int x, int y, int button);
void gui_handle_mouse_move(int x, int y);
void gui_handle_mouse_release(int x, int y, int button);
int gui_is_point_in_rect(int px, int py, int rx, int ry, int rw, int rh);
gui_window_t* gui_get_window_at_point(int x, int y);
gui_button_t* gui_get_button_at_point(int x, int y);
int gui_get_taskbar_button_at_point(int x, int y);
#endif