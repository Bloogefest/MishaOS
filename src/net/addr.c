#include "addr.h"

#include "../stdlib.h"
#include "../string.h"

const eth_addr_t eth_null_addr = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
const eth_addr_t eth_broadcast_addr = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

const ipv4_addr_t ipv4_null_addr = {{0x00, 0x00, 0x00, 0x00}};
const ipv4_addr_t ipv4_broadcast_addr = {{0xFF, 0xFF, 0xFF, 0xFF}};

char* ethtoa(const eth_addr_t* addr, char* str) {
    for (uint8_t i = 0; i < 6; i++) {
        if (addr->address[i] < 0x10) {
            *str++ = '0';
        }

        str = itoa(addr->address[i], str, 16);
        *str++ = ':';
    }

    *--str = 0;
    return str;
}

char* ip4toa(const ipv4_addr_t* addr, char* str) {
    for (uint8_t i = 0; i < 4; i++) {
        str = itoa(addr->address[i], str, 10);
        *str++ = '.';
    }

    *--str = 0;
    return str;
}

char* ip4ptoa(const ipv4_addr_t* addr, uint16_t port, char* str) {
    str = ip4toa(addr, str);
    *str++ = ':';
    return itoa(port, str, 10);
}

uint8_t atoip4(char* str, ipv4_addr_t* ip) {
    for (uint8_t i = 0;; i++) {
        ip->address[i] = atoi(str, 10);

        if (i == 3) {
            return 1;
        }

        if (!(str = _strchr(str, '.', 4))) {
            return 0;
        }

        ++str;
    }
}

uint8_t atoip4p(char* str, ipv4_addr_t* ip, uint16_t* port) {
    if (!atoip4(str, ip)) {
        return 0;
    }

    if (!(str = _strchr(str, ':', 16))) {
        return 0;
    }

    *port = atoi(++str, 10);
    return 1;
}