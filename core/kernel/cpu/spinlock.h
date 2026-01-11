#ifndef SPINLOCK_H
#define SPINLOCK_H
#include <outputs/types.h>
typedef struct {
    volatile u32 locked;
} spinlock_t;
void spinlock_init(spinlock_t* lock);
void spinlock_acquire(spinlock_t* lock);
void spinlock_release(spinlock_t* lock);
int spinlock_try_acquire(spinlock_t* lock);
int spinlock_is_locked(spinlock_t* lock);
#endif