#ifndef CORE_UTILS_H
#define CORE_UTILS_H
#include <outputs/types.h>
static inline void system_halt(void) {
    while (1) {
        __asm__ volatile ("hlt");
    }
}
static inline void wait_cycles(u32 cycles) {
    volatile u32 counter = 0;
    while (counter < cycles * 5000000) {
        __asm__ volatile ("nop");
        counter++;
    }
}
#endif