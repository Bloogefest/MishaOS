#pragma once

#include <stdint.h>
#include <dev/pci.h>

typedef struct driver_s {
    void(*init)(pci_device_info_t* info, uint32_t bus, uint32_t dev, uint32_t func);
} driver_t;

extern const driver_t* PCI_DRIVERS[];