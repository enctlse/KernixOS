#include "string.h"
#include "outputs/print.h"
void str_copy(char *destination, const char *source) {
    char *dest_ptr = destination;
    const char *src_ptr = source;
    while ((*dest_ptr++ = *src_ptr++));
}
void str_append(char *destination, const char *source) {
    char *dest_ptr = destination;
    while (*dest_ptr) dest_ptr++;
    const char *src_ptr = source;
    while ((*dest_ptr++ = *src_ptr++));
}
static void add_digits(char *buffer, u32 number, int *position) {
    if (number >= 10) {
        add_digits(buffer, number / 10, position);
    }
    buffer[(*position)++] = '0' + (number % 10);
}
void str_append_uint(char *destination, u32 number) {
    char temp_buffer[20];
    int pos = 0;
    if (number == 0) {
        temp_buffer[pos++] = '0';
    } else {
        add_digits(temp_buffer, number, &pos);
    }
    temp_buffer[pos] = '\0';
    str_append(destination, temp_buffer);
}
void str_append_hex(char *destination, u64 number) {
    char temp_buffer[17];
    int pos = 0;
    if (number == 0) {
        temp_buffer[pos++] = '0';
    } else {
        for (int shift = 15; shift >= 0; shift--) {
            int digit_value = (number >> (shift * 4)) & 0xF;
            if (digit_value || pos) {
                temp_buffer[pos++] = (digit_value < 10) ? '0' + digit_value : 'a' + (digit_value - 10);
            }
        }
    }
    temp_buffer[pos] = '\0';
    str_append(destination, temp_buffer);
}
int str_len(const char *input_string) {
    const char *ptr = input_string;
    while (*ptr) ptr++;
    return ptr - input_string;
}
int str_starts_with(const char *input_string, const char *prefix_string) {
    if (!input_string || !prefix_string) return 0;
    while (*prefix_string && *input_string == *prefix_string) {
        input_string++;
        prefix_string++;
    }
    return !*prefix_string;
}
int str_equals(const char *first_string, const char *second_string) {
    if (!first_string || !second_string) return 0;
    while (*first_string && *second_string && !(*first_string ^ *second_string)) {
        first_string++;
        second_string++;
    }
    return !(*first_string ^ *second_string);
}
void str_to_upper(char *input_string) {
    if (!input_string) return;
    char *ptr = input_string;
    while (*ptr) {
        *ptr &= ~0x20;
        ptr++;
    }
}
void str_to_lower(char *input_string) {
    if (!input_string) return;
    char *ptr = input_string;
    while (*ptr) {
        *ptr |= 0x20;
        ptr++;
    }
}
void str_trim(char *input_string) {
    if (!input_string) return;
    char *start_pos = input_string;
    while (*start_pos && (*start_pos == ' ' || *start_pos == '\t' || *start_pos == '\n')) start_pos++;
    char *end_pos = start_pos;
    while (*end_pos) end_pos++;
    if (end_pos > start_pos) {
        end_pos--;
        while (end_pos > start_pos && (*end_pos == ' ' || *end_pos == '\t' || *end_pos == '\n')) end_pos--;
        end_pos++;
    }
    char *dest_ptr = input_string;
    while (start_pos < end_pos) *dest_ptr++ = *start_pos++;
    *dest_ptr = '\0';
}
void str_reverse(char *input_string) {
    if (!input_string) return;
    char *end_ptr = input_string;
    while (*end_ptr) end_ptr++;
    end_ptr--;
    while (input_string < end_ptr) {
        *input_string ^= *end_ptr;
        *end_ptr ^= *input_string;
        *input_string ^= *end_ptr;
        input_string++;
        end_ptr--;
    }
}
int str_to_int(const char *input_string) {
    if (!input_string) return 0;
    int result_value = 0;
    int sign_value = 1;
    const char *ptr = input_string;
    while (*ptr == ' ' || *ptr == '\t') ptr++;
    if (*ptr == '-') {
        sign_value = -1;
        ptr++;
    } else if (*ptr == '+') {
        ptr++;
    }
    while (*ptr >= '0' && *ptr <= '9') {
        result_value = result_value * 10 + (*ptr++ - '0');
    }
    return result_value * sign_value;
}
void str_from_int(char *buffer, int value) {
    if (!buffer) return;
    char *buf_ptr = buffer;
    int is_negative = 0;
    if (value < 0) {
        is_negative = 1;
        value = -value;
    }
    do {
        *buf_ptr++ = '0' + (value % 10);
        value /= 10;
    } while (value);
    if (is_negative) *buf_ptr++ = '-';
    *buf_ptr = '\0';
    str_reverse(buffer);
}
int str_contains(const char *input_string, const char *substring) {
    if (!input_string || !substring) return 0;
    const char *str_ptr = input_string;
    while (*str_ptr) {
        const char *sub_ptr = substring;
        const char *temp_ptr = str_ptr;
        while (*temp_ptr && *sub_ptr && *temp_ptr == *sub_ptr) {
            temp_ptr++;
            sub_ptr++;
        }
        if (!*sub_ptr) return 1;
        str_ptr++;
    }
    return 0;
}
void print_str(const char *input_string, u32 color_value) {
    string(input_string, color_value);
    putchar('\n', color_value);
}