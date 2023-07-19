#pragma once

#include <net/ipv4.h>

#define ICMP_TYPE_ECHO_REPLY 0
#define ICMP_TYPE_DEST_UNREACHABLE 3
#define ICMP_TYPE_SOURCE_QUENCH 4
#define ICMP_TYPE_REDIRECT_MSG 5
#define ICMP_TYPE_ECHO_REQUEST 8
#define ICMP_TYPE_ROUTER_ADVERTISEMENT 9
#define ICMP_TYPE_ROUTER_SOLICITATION 10
#define ICMP_TYPE_TIME_EXCEEDED 11
#define ICMP_TYPE_BAD_PARAM 12
#define ICMP_TYPE_TIMESTAMP 13
#define ICMP_TYPE_TIMESTAMP_REPLY 14
#define ICMP_TYPE_INFO_REQUEST 15
#define ICMP_TYPE_INFO_REPLY 16
#define ICMP_TYPE_ADDR_MASK_REQUEST 17
#define ICMP_TYPE_ADDR_MASK_REPLY 18
#define ICMP_TYPE_TRACEROUTE 30

void icmp_recv(net_intf_t* intf, const ipv4_header_t* header, net_buf_t* packet);
void icmp_echo_request(const ipv4_addr_t* addr, uint16_t id, uint16_t seq, const uint8_t* data, const uint8_t* end);