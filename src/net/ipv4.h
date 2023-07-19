#pragma once

#include <net/intf.h>

#define IP_PROTOCOL_ICMP 1
#define IP_PROTOCOL_TCP 6
#define IP_PROTOCOL_UDP 17

typedef struct ipv4_header_s {
    uint8_t ver_ihl;
    uint8_t tos;
    uint16_t len;
    uint16_t id;
    uint16_t offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    ipv4_addr_t src;
    ipv4_addr_t dst;
} __attribute__((packed)) ipv4_header_t;

void ipv4_recv(net_intf_t* intf, net_buf_t* packet);
void ipv4_send(const ipv4_addr_t* dst, uint8_t protocol, net_buf_t* packet);
void ipv4_intf_send(net_intf_t* intf, const ipv4_addr_t* next_addr, const ipv4_addr_t* dst, uint8_t protocol, net_buf_t* packet);
void ipv4_dump(const net_buf_t* packet);