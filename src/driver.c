#include "driver.h"

#include "net/driver/intel.h"

const driver_t INTEL_NET_DRIVER = {.init = intel_net_driver_init};

const driver_t* PCI_DRIVERS[] = {
        &INTEL_NET_DRIVER,
        0
};