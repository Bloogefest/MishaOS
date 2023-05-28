#pragma once

#include "addr.h"
#include "intf.h"

typedef struct arp_header_s {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t op;
} __attribute__((packed)) arp_header_t;

void arp_init();

const eth_addr_t* arp_lookup_eth_addr(const ipv4_addr_t* pa);
void arp_request(net_intf_t* intf, const ipv4_addr_t* tpa, uint16_t ethertype, net_buf_t* packet);
void arp_reply(net_intf_t* intf, const eth_addr_t* ha, const ipv4_addr_t* pa);

void arp_recv(net_intf_t* intf, net_buf_t* packet);