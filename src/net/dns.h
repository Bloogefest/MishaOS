#pragma once

#include <net/intf.h>

typedef struct dns_header_s {
    uint16_t id;
    uint16_t flags;
    uint16_t question_count;
    uint16_t answer_count;
    uint16_t authority_count;
    uint16_t additional_count;
} __attribute__((packed)) dns_header_t;

extern ipv4_addr_t dns_server;

typedef void(*dns_callback_t)(void* ctx, const char* host, const net_buf_t* buf);

uint8_t dns_get_ip4_a(ipv4_addr_t* addr, const net_buf_t* buf);

void dns_recv(net_intf_t* intf, const net_buf_t* buf);
void dns_query_host(const char* host, uint32_t id, void* ctx, dns_callback_t callback);
void dns_dump(const net_buf_t* buf);