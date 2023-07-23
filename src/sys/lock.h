#pragma once

#include <stdint.h>

void spin_lock(volatile uint8_t* lock);
void spin_unlock(volatile uint8_t* lock);