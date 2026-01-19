#include <gui/gui.h>
#include <string/string.h>
#include <kernel/display/visual.h>
#include <kernel/display/fonts/typeface.h>

void gui_draw_settings(gui_window_t* window) {
    if (!window || !str_equals(window->title, "Settings")) return;
    int mx = window->x;
    int my = window->y;
    int mw = window->width;
    int mh = window->height;
    // Draw background only for content area, not covering title
    int bg_x = mx;
    int bg_y = my + 25; // WINDOW_TITLE_HEIGHT
    int bg_w = mw;
    int bg_h = mh - 25;
    if (bg_x >= 0 && bg_y >= 0 && bg_w > 0 && bg_h > 0) {
        draw_rect(bg_x, bg_y, bg_w, bg_h, 0xFFFFFFFF);
    }
}