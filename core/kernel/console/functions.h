#pragma once
#include "console.h"
#include <types.h>
#define BANNER_HEIGHT 15
#define BANNER_Y_SPACING 4
#define BANNER_BG_COLOR COLOR_BLACK
#define BANNER_BORDER_COLOR GFX_GRAY_40
#define BANNER_TEXT_COLOR CONSOLESCREEN_COLOR
#define BANNER_UPDATE_INTERVAL 60
#define BANNER_OSNAME_COLOR GFX_CYAN
#define CONSOLE_PADDING_X 0
void banner_init(void);
void banner_draw(void);
void banner_update_time(void);
void banner_tick(void);
void banner_force_update(void);
u32 banner_get_height(void);
void console_window_init(void);
void console_window_clear(u32 color);
u32 console_window_get_start_y(void);
u32 console_window_get_max_y(void);
void console_window_check_scroll(void);
void console_window_update_layout(void);
void console_window_scroll_lines(u32 lines);
u32 console_window_get_visible_lines(void);
int console_window_needs_scroll(void);
FHDR(cmd_echo);
FHDR(cmd_clear);
FHDR(cmd_help);
FHDR(cmd_fsize);
FHDR(cmd_modules);
FHDR(cmd_meminfo);
FHDR(cmd_memtest);
FHDR(cmd_sysinfo);
FHDR(cmd_cal);
FHDR(cmd_date);
FHDR(cmd_time);
FHDR(cmd_uptime);
FHDR(cmd_cat);
FHDR(cmd_ls);
FHDR(cmd_cd);
FHDR(cmd_mkdir);
FHDR(cmd_touch);
FHDR(cmd_poweroff);
FHDR(cmd_reboot);
FHDR(cmd_shutdown);
FHDR(cmd_gui);
FHDR(cmd_loadcursor);
FHDR(cmd_mouse);
FHDR(cmd_mouse);
FHDR(cmd_mousemove);
FHDR(cmd_cls);
FHDR(cmd_ping);
FHDR(cmd_cpuinfo);