#ifndef ASSEMBLY_H
#define ASSEMBLY_H
#include <outputs/types.h>
static inline void hcf(void)
{
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
static inline void delay(u32 count)
{
    for (volatile u32 i = 0; i < count * 10000000; i++)
    {
        __asm__ volatile ("nop");
    }
}
#endif