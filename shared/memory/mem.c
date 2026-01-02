#include "main.h"
#include <kernel/mem/heap/heap.h>
#include <stdint.h>
#define BLOCK_SIZE 32
void memset(void *ptr, u8 val, size_t n)
{
    u8 *p = (u8 *)ptr;
    u32 val32 = val | (val << 8) | (val << 16) | (val << 24);
    while (n && ((uintptr_t)p & 3)) {
        *p++ = val;
        n--;
    }
    u32 *p32 = (u32 *)p;
    size_t n32 = n / 4;
    while (n32--) {
        *p32++ = val32;
    }
    p = (u8 *)p32;
    n %= 4;
    while (n--) {
        *p++ = val;
    }
}
void memcpy(void *dst, const void *src, size_t n)
{
    u8 *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    while (n && ((uintptr_t)d & 3)) {
        *d++ = *s++;
        n--;
    }
    u32 *d32 = (u32 *)d;
    const u32 *s32 = (const u32 *)s;
    size_t n32 = n / 4;
    while (n32--) {
        *d32++ = *s32++;
    }
    d = (u8 *)d32;
    s = (const u8 *)s32;
    n %= 4;
    while (n--) {
        *d++ = *s++;
    }
}
void memmove(void *dst, const void *src, size_t n)
{
    u8 *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    if (d < s) {
        for (size_t i = 0; i < n; i++)
            d[i] = s[i];
    } else {
        for (size_t i = n; i > 0; i--)
            d[i - 1] = s[i - 1];
    }
}
int memcmp(const void *a, const void *b, size_t n)
{
    const u8 *pa = (const u8 *)a;
    const u8 *pb = (const u8 *)b;
    for (size_t i = 0; i < n; i++) {
        if (pa[i] != pb[i])
            return pa[i] - pb[i];
    }
    return 0;
}