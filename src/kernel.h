#pragma once

#include <stdint.h>

typedef struct kernel_meminfo_s {
    uint32_t kernel_physical_start;
    uint32_t kernel_physical_end;
    uint32_t kernel_virtual_start;
    uint32_t kernel_virtual_end;
} __attribute__((packed)) kernel_meminfo_t;

typedef struct kernel_func_info_s {
    uint16_t len;
    uint32_t start_addr;
    uint32_t end_addr;
    char func_name[];
} __attribute__((packed)) kernel_func_info_t;

extern kernel_func_info_t* kernel_funcs;

const char* kernel_get_func_name(uintptr_t addr);