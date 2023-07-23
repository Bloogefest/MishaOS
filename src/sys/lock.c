#include "lock.h"

#include <sys/process.h>

void spin_lock(volatile uint8_t* lock) {
    while(__sync_lock_test_and_set(lock, 0x01)) {
        switch_task(1);
    }
}

void spin_unlock(volatile uint8_t* lock) {
    __sync_lock_release(lock);
}