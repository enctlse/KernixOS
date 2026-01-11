#include "ui/theme/colors.h"
#include <fs/vfs/vfs.h>
#include <kernel/communication/serial.h>
#include <string/string.h>
#define PCF_FONT_PATH "/boot/fonts/prefs.psf"
extern int init_boot_log;
#define BOOTUP_VISUALS 0
#if BOOTUP_VISUALS == 0
    #define BOOTUP_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
    #define BOOTUP_PRINT(msg, col) \
        do { \
            print(msg, col); \
            if (init_boot_log >= 0) fs_write(init_boot_log, msg, str_len(msg)); \
        } while(0)
    #define BOOTUP_PRINT_INT(num, col) \
        do { \
            printInt(num, col); \
            if (init_boot_log >= 0) { \
                char buf[12]; \
                buf[0] = '\0'; \
                str_append_uint(buf, (u32)num); \
                fs_write(init_boot_log, buf, str_len(buf)); \
            } \
        } while(0)
    #define BOOTUP_PRINTBS(msg, col) \
        do { \
            printbs(msg, col); \
            if (init_boot_log >= 0) fs_write(init_boot_log, msg, str_len(msg)); \
        } while(0)
#else
    #define BOOTUP_PRINTF(fmt, ...) ((void)0)
    #define BOOTUP_PRINT(msg, col) \
        do { \
            printf("%s", msg); \
            if (init_boot_log >= 0) fs_write(init_boot_log, msg, str_len(msg)); \
        } while(0)
    #define BOOTUP_PRINT_INT(num, col) \
        do { \
            printInt(num, col); \
            if (init_boot_log >= 0) { \
                char buf[12]; \
                buf[0] = '\0';
                str_append_uint(buf, num); \
                fs_write(init_boot_log, buf, str_len(buf)); \
            } \
        } while(0)
    #define BOOTUP_PRINTBS(msg, col) \
        do { \
            printf("%s", msg); \
            if (init_boot_log >= 0) fs_write(init_boot_log, msg, str_len(msg)); \
        } while(0)
#endif
#define BOOTSCREEN_BG_COLOR st_black
#define BOOTSCREEN_COLOR st_white
#define BOOTUP_COLOR_THEME STD
#define CONSOLESCREEN_BG_COLOR 0x000000
#define CONSOLESCREEN_COLOR white
#define CONSOLE_COLOR_THEME FLU
#define PANICSCREEN_BG_COLOR black
#define PANICSCREEN_COLOR st_white
#define PANICSCREEN_COLOR_R st_red
#define PANIC_COLOR_THEME STD
