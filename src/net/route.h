#pragma once

#include "intf.h"

typedef struct net_route_s {
    struct net_route_s* next;
    struct net_route_s* prev;
    ipv4_addr_t dst;
    ipv4_addr_t mask;
    ipv4_addr_t gateway;
    net_intf_t* intf;
} net_route_t;

const net_route_t* net_find_route(const ipv4_addr_t* dst);
void net_add_route(const ipv4_addr_t* dst, const ipv4_addr_t* mask, const ipv4_addr_t* gateway, net_intf_t* intf);
const ipv4_addr_t* net_next_addr(const net_route_t* route, const ipv4_addr_t* dst);
void net_route_table_dump();