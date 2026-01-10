#ifndef MAIN_H
#define MAIN_H
#include <types.h>
void memset(void *ptr, u8 val, size_t n);
void memcpy(void *dst, const void *src, size_t n);
void memmove(void *dst, const void *src, size_t n);
int memcmp(const void *a, const void *b, size_t n);
u64 mem_get_free(void);
u64 mem_get_used(void);
u64 mem_get_total(void);
void* kmalloc_dma(size_t size, size_t alignment);
void kfree_dma(void *ptr);
void* kmalloc_aligned(size_t size, size_t alignment);
#endif