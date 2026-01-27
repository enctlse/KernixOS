#include <gui/gui.h>
#include <string/string.h>
#include <kernel/display/visual.h>
#include <kernel/display/fonts/typeface.h>
#include <fs/vfs/vfs.h>
#define MAX_ENTRIES 100
#define ENTRY_HEIGHT 20
#define ICON_SIZE 16
gui_file_manager_state_t file_manager_states[MAX_WINDOWS];
static int file_manager_state_used[MAX_WINDOWS] = {0};
void gui_file_manager_init(gui_file_manager_state_t* state) {
    str_copy(state->current_path, "/");
    state->entry_count = 0;
    state->selected_index = -1;
    state->scroll_offset = 0;
    gui_file_manager_load_directory(state, "/");
}
void gui_file_manager_load_directory(gui_file_manager_state_t* state, const char* path) {
    str_copy(state->current_path, path);
    state->entry_count = 0;
    state->selected_index = -1;
    state->scroll_offset = 0;
    fs_node* dir_node = fs_resolve(path);
    if (!dir_node || dir_node->type != FS_DIR) {
        return;
    }
    if (str_len(path) > 1) {
        str_copy(state->entries[state->entry_count++], "..");
    }
    fs_node* child = dir_node->children;
    while (child && state->entry_count < GUI_FILE_MANAGER_MAX_ENTRIES) {
        str_copy(state->entries[state->entry_count++], child->name);
        child = child->next;
    }
}
void gui_draw_file_manager(gui_window_t* window) {
    if (!window || !str_equals(window->title, "File Manager") || !window->file_manager_state) return;
    gui_file_manager_state_t* state = window->file_manager_state;
    int mx = window->x;
    int my = window->y;
    int mw = window->width;
    int mh = window->height;
    draw_rect(mx, my + 25, mw, mh - 25, 0xFFFFFFFF);
    draw_rect(mx, my + 25, mw, 25, 0xFFE0E0E0);
    gui_draw_text_line_fast(state->current_path, mx + 10, my + 30, 0xFF000000);
    int content_y = my + 50;
    int content_h = mh - 50;
    int visible_entries = content_h / ENTRY_HEIGHT;
    for (int i = state->scroll_offset; i < state->entry_count && i < state->scroll_offset + visible_entries; i++) {
        int entry_y = content_y + (i - state->scroll_offset) * ENTRY_HEIGHT;
        u32 bg_color = (i == state->selected_index) ? 0xFFADD8E6 : 0xFFFFFFFF;
        draw_rect(mx, entry_y, mw, ENTRY_HEIGHT, bg_color);
        draw_rect(mx + 5, entry_y + 2, ICON_SIZE, ICON_SIZE, 0xFF808080);
        gui_draw_text_line_fast(state->entries[i], mx + 30, entry_y + 4, 0xFF000000);
    }
}
void gui_file_manager_handle_click(gui_file_manager_state_t* state, int x, int y, gui_window_t* window) {
    int content_y = window->y + 50;
    int content_h = window->height - 50;
    int clicked_y = y - content_y;
    if (clicked_y < 0) return;
    int clicked_index = (clicked_y / ENTRY_HEIGHT) + state->scroll_offset;
    if (clicked_index >= state->entry_count) return;
    state->selected_index = clicked_index;
    const char* selected_name = state->entries[clicked_index];
    if (str_equals(selected_name, "..")) {
        char parent_path[GUI_FILE_MANAGER_MAX_PATH];
        str_copy(parent_path, state->current_path);
        int len = str_len(parent_path);
        if (len > 1) {
            while (len > 1 && parent_path[len - 1] != '/') {
                len--;
            }
            if (len > 1) len--;
            parent_path[len] = '\0';
            if (len == 0) str_copy(parent_path, "/");
            gui_file_manager_load_directory(state, parent_path);
        }
    } else {
        char full_path[GUI_FILE_MANAGER_MAX_PATH * 2];
        str_copy(full_path, state->current_path);
        if (!str_equals(state->current_path, "/")) {
            str_append(full_path, "/");
        }
        str_append(full_path, selected_name);
        fs_node* node = fs_resolve(full_path);
        if (node && node->type == FS_DIR) {
            gui_file_manager_load_directory(state, full_path);
        }
    }
}