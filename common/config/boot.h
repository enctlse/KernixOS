#include "ui/theme/colors.h"
#include <fs/vfs/vfs.h>
#include <kernel/communication/serial.h>
#include <string/string.h>
#define PCF_FONT_PATH "/boot/fonts/prefs.psf"
extern int init_boot_log;
#define BOOTUP_VISUALS 0
#if BOOTUP_VISUALS == 0
    #define BOOTUP_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
    #define SYSTEM_PRINT(msg, col) \
        do { \
            print(msg, col); \
            if (init_boot_log >= 0) fs_write(init_boot_log, msg, str_len(msg)); \
        } while(0)
    #define SYSTEM_PRINT_INT(num, col) \
        do { \
            printInt(num, col); \
            if (init_boot_log >= 0) { \
                char buf[12]; \
                buf[0] = '\0'; \
                str_append_uint(buf, (u32)num); \
                fs_write(init_boot_log, buf, str_len(buf)); \
            } \
        } while(0)
    #define SYSTEM_PRINTBS(msg, col) \
        do { \
            printbs(msg, col); \
            if (init_boot_log >= 0) fs_write(init_boot_log, msg, str_len(msg)); \
        } while(0)
    #define SYSTEM_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
    #define BOOTUP_PRINTF(fmt, ...) ((void)0)
    #define SYSTEM_PRINT(msg, col) \
        do { \
            printf("%s", msg); \
            if (init_boot_log >= 0) fs_write(init_boot_log, msg, str_len(msg)); \
        } while(0)
    #define SYSTEM_PRINT_INT(num, col) \
        do { \
            printInt(num, col); \
            if (init_boot_log >= 0) { \
                char buf[12]; \
                buf[0] = '\0';
                str_append_uint(buf, num); \
                fs_write(init_boot_log, buf, str_len(buf)); \
            } \
        } while(0)
    #define SYSTEM_PRINTBS(msg, col) \
        do { \
            printf("%s", msg); \
            if (init_boot_log >= 0) fs_write(init_boot_log, msg, str_len(msg)); \
        } while(0)
#endif
#define BOOTSCREEN_BG_COLOR 0x000000
#define BOOTSCREEN_COLOR 0xFFFFFF
#define BOOTUP_COLOR_THEME STD
#define CONSOLESCREEN_BG_COLOR 0x000000
#define CONSOLESCREEN_COLOR 0xFFFFFF
#define CONSOLE_COLOR_THEME FLU
#define PANICSCREEN_BG_COLOR 0x000000
#define PANICSCREEN_COLOR 0xFFFFFF
#define PANICSCREEN_COLOR_R 0xFF8B0000
#define PANIC_COLOR_THEME STD
#define theme_white 0xFFFFFF
#define gray_70 0xB8B8B8
#define green 0xFF00FF00
#define red 0xFF8B0000
#define cyan 0xFF00FFFF
#define yellow 0xFFFFFF00
#define blue 0xFF0000FF
#define purple 0xFF800080
#define black 0x000000
#define theme_bg 0x000000
#define theme_red 0xFF8B0000
#define theme_green 0xFF00FF00
#define theme_blue 0xFF0000FF
#define theme_yellow 0xFFFFFF00
#define theme_cyan 0xFF00FFFF
#define theme_purple 0xFF800080
#define theme_black 0x000000
#define st_white 0xFFFFFF
#define st_red 0xFF8B0000
#define st_cyan 0xFF00FFFF
#define st_black 0x000000
#define theme_init() ((void)0)
#define setcontext(x) ((void)0)
#define sbootup_theme(x) ((void)0)
#define sconsole_theme(x) ((void)0)
#define spanic_theme(x) ((void)0)