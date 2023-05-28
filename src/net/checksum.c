#include "checksum.h"

uint16_t net_checksum(const uint8_t* data, const uint8_t* end) {
    return net_checksum_final(net_checksum_update(data, end, 0));
}

uint32_t net_checksum_update(const uint8_t* data, const uint8_t* end, uint32_t sum) {
    uint32_t len = end - data;
    uint16_t* ptr = (uint16_t*) data;
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }

    if (len) {
        sum += *(uint8_t*) ptr;
    }

    return sum;
}
uint16_t net_checksum_final(uint32_t sum) {
    sum = (sum & 0xFFFF) + (sum >> 16);
    sum += (sum >> 16);

    uint16_t n = ~sum;
    return ((n & 0x00FF) << 8) | ((n & 0xFF00) >> 8);
}