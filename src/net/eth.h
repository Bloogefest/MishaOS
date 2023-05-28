#pragma once

#include <stdint.h>
#include "intf.h"

#define ET_IPV4 0x0800
#define ET_ARP 0x806
#define ET_IPV6 0x86DD

typedef struct eth_header_s {
    eth_addr_t dst;
    eth_addr_t src;
    uint16_t ethertype;
} __attribute__((packed)) eth_header_t;

typedef struct eth_packet_s {
    eth_header_t* header;
    uint16_t ethertype;
    uint16_t header_len;
} eth_packet_t;

void eth_recv(net_intf_t* intf, net_buf_t* packet);
void eth_intf_send(net_intf_t* intf, const void* dst, uint16_t ethertype, net_buf_t* packet);
void eth_dump(net_buf_t* packet);