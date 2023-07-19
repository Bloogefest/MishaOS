#pragma once

#include <stdint.h>
#include <stddef.h>

void heap_init(void* heap_address, size_t page_count);
void heap_expand(size_t length);

void* malloc(size_t size);
void free(void* address);