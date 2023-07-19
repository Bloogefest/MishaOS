#pragma once

#include <net/intf.h>

void ntp_recv(net_intf_t* intf, const net_buf_t* packet);
void ntp_send(const ipv4_addr_t* dst_addr);
void ntp_dump(const net_buf_t* packet);