#pragma once

#include <stdint.h>

typedef struct smap_entry_s {
    uint32_t base1;
    uint32_t base2;
    uint32_t length1;
    uint32_t length2;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) smap_entry_t;

#define MEMORY_TYPE_USABLE 1
#define MEMORY_TYPE_RESERVED 2
#define MEMORY_TYPE_ACPI_RECLAIMABLE 3
#define MEMORY_TYPE_ACPI_NVS 4
#define MEMORY_TYPE_BAD_MEMORY 5

__attribute__((noinline)) __attribute__((regparm(3)))
int detect_memory(smap_entry_t* buffer, int count);