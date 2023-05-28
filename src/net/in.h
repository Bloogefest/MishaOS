#pragma once


#include <stdint.h>

static inline uint16_t net_swap16(uint16_t s) {
    return (s >> 8) | (s << 8);
}

static inline uint32_t net_swap32(uint32_t l) {
    return __builtin_bswap32(l);
}

static inline uint64_t net_swap64(uint64_t ll) {
    return __builtin_bswap64(ll);
}