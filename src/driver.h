#pragma once

#include <stdint.h>

typedef struct driver_s {
    void(*init)(uint32_t bus, uint32_t dev, uint32_t func);
} driver_t;

extern const driver_t** PCI_DRIVER_TABLE[];