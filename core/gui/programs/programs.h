#ifndef GUI_PROGRAMS_H
#define GUI_PROGRAMS_H
#include <kernel/shell/acsh.h>
#include <gui/gui.h>
#define GUI_TEXT_EDITOR_LINES 100
#define GUI_TEXT_EDITOR_COLS 80
#define GUI_OUTPUT_LINES 100
#define GUI_OUTPUT_COLS 256
#define GUI_FILE_MANAGER_MAX_ENTRIES 100
#define GUI_FILE_MANAGER_MAX_PATH 256
typedef struct {
    char lines[GUI_TEXT_EDITOR_LINES][GUI_TEXT_EDITOR_COLS];
    int line_count;
    int cursor_line;
    int cursor_col;
    int scroll_offset;
} gui_text_editor_state_t;
typedef struct {
    char current_path[GUI_FILE_MANAGER_MAX_PATH];
    char entries[GUI_FILE_MANAGER_MAX_ENTRIES][256];
    int entry_count;
    int selected_index;
    int scroll_offset;
} gui_file_manager_state_t;
FHDR(cmd_gui);
FHDR(cmd_calc);
FHDR(cmd_edit);
void gui_draw_settings(gui_window_t* window);
void gui_draw_text_editor(gui_window_t* window);
void gui_text_editor_handle_key(gui_text_editor_state_t* state, int key);
void gui_text_editor_init(gui_text_editor_state_t* state);
void gui_draw_file_manager(gui_window_t* window);
void gui_file_manager_init(gui_file_manager_state_t* state);
void gui_file_manager_load_directory(gui_file_manager_state_t* state, const char* path);
void gui_file_manager_handle_click(gui_file_manager_state_t* state, int x, int y, gui_window_t* window);
#endif