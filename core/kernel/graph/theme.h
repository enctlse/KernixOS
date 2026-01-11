#ifndef THEME_H
#define THEME_H
#include <outputs/types.h>
#ifdef __cplusplus
extern "C" {
#endif
#define THEME_OSNAME_COLOR cyan
typedef enum {
    THEME_STD = 0,
    THEME_FLU = 1
} ThemeType;
typedef enum {
    THEME_BOOTUP = 0,
    THEME_CONSOLE = 1,
    THEME_PANIC = 2
} ThemeContext;
typedef enum {
    COLOR_BLACK,
    COLOR_BG,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_PURPLE,
    COLOR_CYAN,
    COLOR_WHITE
} ThemeColor;
typedef struct {
    u32 BLACK;
    u32 BG;
    u32 RED;
    u32 GREEN;
    u32 YELLOW;
    u32 BLUE;
    u32 PURPLE;
    u32 CYAN;
    u32 WHITE;
} ThemeColors;
void theme_init();
void setcontext(ThemeContext context);
ThemeContext getcontext();
void sbootup_theme(ThemeType type);
void sconsole_theme(ThemeType type);
void spanic_theme(ThemeType type);
u32 get_color(ThemeColor color);
u32 theme_black();
u32 theme_bg();
u32 theme_red();
u32 theme_green();
u32 theme_yellow();
u32 theme_blue();
u32 theme_purple();
u32 theme_cyan();
u32 theme_white();
#ifdef __cplusplus
}
#endif
#endif