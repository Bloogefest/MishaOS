#pragma once

#include "intf.h"

extern ipv4_addr_t dns_server;

void dns_recv(net_intf_t* intf, const net_buf_t* buf);
void dns_query_host(const char* host, uint32_t id);
void dns_dump(const net_buf_t* buf);