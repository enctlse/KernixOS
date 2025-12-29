#ifndef CONSOLE_UNO_H
#define CONSOLE_UNO_H
#include <types.h>
#include <theme/stdclrs.h>
#include <theme/tmx.h>
#define BANNER_HEIGHT 15
#define BANNER_Y_SPACING 4
#define BANNER_BG_COLOR COLOR_BLACK
#define BANNER_BORDER_COLOR GFX_GRAY_40
#define BANNER_TEXT_COLOR CONSOLESCREEN_COLOR
#define BANNER_UPDATE_INTERVAL 60
#define BANNER_OSNAME_COLOR GFX_CYAN
void banner_init(void);
void banner_draw(void);
void banner_update_time(void);
void banner_tick(void);
void banner_force_update(void);
u32 banner_get_height(void);
#endif