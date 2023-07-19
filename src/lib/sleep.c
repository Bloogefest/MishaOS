#include "sleep.h"

#include <sys/pit.h>

uint32_t sleep(uint32_t seconds) {
    return pit_sleep((uint64_t) seconds * 1000);
}

uint32_t pit_sleep(uint64_t ms) {
    uint64_t ticks = pit_get_ticks() + ms;
    while (pit_get_ticks() < ticks) {
        asm("hlt");
    }

    return 0;
}