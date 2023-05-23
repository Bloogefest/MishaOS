#include "pit.h"
#include "io.h"

static uint64_t ticks = 0;

void pit_set_phase(uint32_t hz) {
    uint32_t divisor = 1193180 / hz;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

void pit_tick() {
    ++ticks;
}

uint64_t pit_get_ticks() {
    return ticks;
}