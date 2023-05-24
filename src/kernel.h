#pragma once

#include <stdint.h>

typedef struct kernel_meminfo_s {
    uint32_t kernel_physical_start;
    uint32_t kernel_physical_end;
    uint32_t kernel_virtual_start;
    uint32_t kernel_virtual_end;
} __attribute__((packed)) kernel_meminfo_t;