#ifndef PRINT_H
#define PRINT_H
#include <outputs/types.h>
void putchar(char c, u32 color);
void print(const char *str, u32 color);
void string(const char *str, u32 color);
void println(const char *str, u32 color);
void print_uint(u32 num, u32 color);
void print_int(int num, u32 color);
void print_hex(u32 num, u32 color);
void print_hex64(u64 num, u32 color);
void print_colored(const char *str, u32 fg_color, u32 bg_color);
void set_cursor(u32 x, u32 y);
void get_cursor(u32 *x, u32 *y);
void printf_simple(const char *fmt, u32 color, ...);
void putchar_bootstrap(char c, u32 color);
void printbs(const char *str, u32 color);
#define printInt(num, color)     print_int(num, color)
#define printUint(num, color)    print_uint(num, color)
#define printHex(num, color)     print_hex(num, color)
#define printHex64(num, color)   print_hex64(num, color)
#define printString(str, color)  print(str, color)
#endif