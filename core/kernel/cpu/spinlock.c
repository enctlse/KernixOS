#include "spinlock.h"
void spinlock_init(spinlock_t* lock) {
    lock->locked = 0;
}
void spinlock_acquire(spinlock_t* lock) {
    while (1) {
        if (__sync_bool_compare_and_swap(&lock->locked, 0, 1)) {
            __asm__ volatile("mfence" ::: "memory");
            return;
        }
        __asm__ volatile("pause");
    }
}
void spinlock_release(spinlock_t* lock) {
    __asm__ volatile("mfence" ::: "memory");
    lock->locked = 0;
}
int spinlock_try_acquire(spinlock_t* lock) {
    if (__sync_bool_compare_and_swap(&lock->locked, 0, 1)) {
        __asm__ volatile("mfence" ::: "memory");
        return 1;
    }
    return 0;
}
int spinlock_is_locked(spinlock_t* lock) {
    return lock->locked != 0;
}