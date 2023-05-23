#pragma once

#include <stdint.h>

void pit_set_phase(uint32_t hz);
void pit_tick();
uint64_t pit_get_ticks();