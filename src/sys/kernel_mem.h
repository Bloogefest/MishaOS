#pragma once

#include <cpu/paging.h>

__attribute__((aligned(4096)))
extern page_directory_t page_directory;
extern pfa_t pfa;