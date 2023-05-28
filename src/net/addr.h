#pragma once

#include <stdint.h>

typedef struct eth_addr_s {
    uint8_t address[6];
} __attribute__((packed)) eth_addr_t;

typedef union ipv4_addr_u {
    uint8_t address[4];
    uint32_t bits;
} __attribute__((packed)) ipv4_addr_t;

extern const eth_addr_t eth_null_addr;
extern const eth_addr_t eth_broadcast_addr;

extern const ipv4_addr_t ipv4_null_addr;
extern const ipv4_addr_t ipv4_broadcast_addr;

char* ethtoa(const eth_addr_t* addr, char* str);
char* ip4toa(const ipv4_addr_t* addr, char* str);
char* ip4ptoa(const ipv4_addr_t* addr, uint16_t port, char* str);

uint8_t atoip4(char* str, ipv4_addr_t* ip);
uint8_t atoip4p(char* str, ipv4_addr_t* ip, uint16_t* port);