#pragma once

#include <stdint.h>
#include <stddef.h>

#define HEAP_START 0x80000000
#define HEAP_END 0x90000000

#define HEAP_START_TABLE (HEAP_START / 1024 / 0x1000)
#define HEAP_END_TABLE (HEAP_END / 1024 / 0x1000)

void heap_init(size_t page_count);
void heap_expand(size_t length);

void heap_map_user_segment(void* address); // Remaps segment to make it accessible from userspace
void heap_map_kernel_segment(void* address); // Remaps segment back to kernel-space only

void* malloc(size_t size);
void free(void* address);