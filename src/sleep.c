#include "sleep.h"
#include "pit.h"

uint32_t sleep(uint32_t seconds) {
    return pit_sleep((uint64_t) seconds * 100);
}

uint32_t pit_sleep(uint64_t tens_of_ms) {
    uint64_t ticks = pit_get_ticks() + tens_of_ms;
    while (pit_get_ticks() < ticks) {
        asm("hlt");
    }

    return 0;
}