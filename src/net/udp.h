#pragma once

#include <net/ipv4.h>

typedef struct udp_header_s {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t len;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

void udp_recv(net_intf_t* intf, const ipv4_header_t* ip_header, net_buf_t* packet);
void udp_send(const ipv4_addr_t* dst_addr, uint32_t dst_port, uint32_t src_port, net_buf_t* packet);
void udp_intf_send(net_intf_t* intf, const ipv4_addr_t* dst_addr, uint32_t dst_port, uint32_t src_port, net_buf_t* packet);
void udp_dump(const net_buf_t* packet);