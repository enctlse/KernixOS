#ifndef STRING_H
#define STRING_H
#include <outputs/types.h>
#include "outputs/print.h"
void str_copy(char *dest, const char *src);
void str_append(char *dest, const char *src);
void str_append_uint(char *dest, u32 num);
void str_append_hex(char *dest, u64 num);
int str_len(const char *str);
void str_reverse(char *str);
void print_str(const char *str, u32 color);
int str_starts_with(const char *str, const char *prefix);
int str_equals(const char *s1, const char *s2);
int str_contains(const char *str, const char *substr);
#endif