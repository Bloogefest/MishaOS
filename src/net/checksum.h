#pragma once

#include <stdint.h>

uint16_t net_checksum(const uint8_t* data, const uint8_t* end);
uint32_t net_checksum_update(const uint8_t* data, const uint8_t* end, uint32_t sum);
uint16_t net_checksum_final(uint32_t sum);