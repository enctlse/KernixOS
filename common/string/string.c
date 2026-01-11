#include "string.h"
#include "outputs/print.h"

void str_copy(char *dest, const char *src)
{
    int index = 0;
    while (src[index] != '\0')
    {
        dest[index] = src[index];
        index++;
    }
    dest[index] = '\0';
}

void str_append(char *dest, const char *src)
{
    int dest_index = 0;
    while (dest[dest_index] != '\0')
    {
        dest_index++;
    }
    int src_index = 0;
    while (src[src_index] != '\0')
    {
        dest[dest_index] = src[src_index];
        dest_index++;
        src_index++;
    }
    dest[dest_index] = '\0';
}

void str_append_uint(char *dest, u32 num)
{
    char buffer[20];
    int buffer_index = 0;
    if (num == 0)
    {
        buffer[buffer_index++] = '0';
    }
    else
    {
        u32 temp_num = num;
        int digit_count = 0;
        while (temp_num > 0)
        {
            temp_num /= 10;
            digit_count++;
        }
        buffer_index = digit_count;
        while (num > 0)
        {
            buffer[--buffer_index] = '0' + (num % 10);
            num /= 10;
        }
        buffer_index = digit_count;
    }
    buffer[buffer_index] = '\0';
    str_append(dest, buffer);
}

int str_len(const char *str)
{
    int length = 0;
    while (str[length] != '\0')
    {
        length++;
    }
    return length;
}
int str_starts_with(const char *str, const char *prefix)
{
    if (!str || !prefix) return 0;
    while (*prefix) {
        if (*str != *prefix) {
            return 0;
        }
        str++;
        prefix++;
    }
    return 1;
}

int str_equals(const char *s1, const char *s2)
{
    if (!s1 || !s2) return 0;
    while (*s1 && *s2) {
        if (*s1 != *s2) return 0;
        s1++;
        s2++;
    }
    return (*s1 == *s2);
}

void str_to_upper(char *str)
{
    if (!str) return;
    while (*str) {
        if (*str >= 'a' && *str <= 'z') {
            *str = *str - 32;
        }
        str++;
    }
}

void str_to_lower(char *str)
{
    if (!str) return;
    while (*str) {
        if (*str >= 'A' && *str <= 'Z') {
            *str = *str + 32;
        }
        str++;
    }
}

void str_trim(char *str)
{
    if (!str) return;
    char *start_ptr = str;
    while (*start_ptr == ' ' || *start_ptr == '\t' || *start_ptr == '\n') {
        start_ptr++;
    }
    char *end_ptr = start_ptr;
    while (*end_ptr) end_ptr++;
    end_ptr--;
    while (end_ptr > start_ptr && (*end_ptr == ' ' || *end_ptr == '\t' || *end_ptr == '\n')) {
        end_ptr--;
    }
    int new_index = 0;
    while (start_ptr <= end_ptr) {
        str[new_index++] = *start_ptr++;
    }
    str[new_index] = '\0';
}

void str_reverse(char *str)
{
    if (!str) return;
    int length = str_len(str);
    for (int idx = 0; idx < length / 2; idx++) {
        char temp_char = str[idx];
        str[idx] = str[length - 1 - idx];
        str[length - 1 - idx] = temp_char;
    }
}

int str_to_int(const char *str)
{
    if (!str) return 0;
    int result = 0;
    int sign = 1;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result * sign;
}

void str_from_int(char *buf, int value)
{
    if (!buf) return;
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    int is_negative = 0;
    if (value < 0) {
        is_negative = 1;
        value = -value;
    }
    int buf_index = 0;
    while (value > 0) {
        buf[buf_index++] = '0' + (value % 10);
        value /= 10;
    }
    if (is_negative) {
        buf[buf_index++] = '-';
    }
    buf[buf_index] = '\0';
    str_reverse(buf);
}

int str_contains(const char *str, const char *substr)
{
    if (!str || !substr) return 0;
    int str_length = 0;
    while (str[str_length]) str_length++;
    int substr_length = 0;
    while (substr[substr_length]) substr_length++;
    if (substr_length > str_length) return 0;
    for (int start_idx = 0; start_idx <= str_length - substr_length; start_idx++)
    {
        int is_match = 1;
        for (int sub_idx = 0; sub_idx < substr_length; sub_idx++)
        {
            if (str[start_idx + sub_idx] != substr[sub_idx])
            {
                is_match = 0;
                break;
            }
        }
        if (is_match) return 1;
    }
    return 0;
}

void print_str(const char *str, u32 color){
    string(str, color);
    putchar('\n', color);
}