#pragma once

#include <net/intf.h>

void dhcp_recv(net_intf_t* intf, const net_buf_t* packet);
void dhcp_discover(net_intf_t* intf);

void dhcp_dump(const net_buf_t* packet);