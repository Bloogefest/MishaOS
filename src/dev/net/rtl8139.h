#pragma once

#include <dev/pci.h>

void rtl8139_driver_init(pci_device_info_t* info, uint32_t bus, uint32_t dev, uint32_t func);