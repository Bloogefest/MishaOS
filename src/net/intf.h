#pragma once

#include <net/addr.h>
#include <net/buf.h>

typedef struct net_intf_s {
    struct net_intf_s* prev;
    struct net_intf_s* next;
    eth_addr_t eth_addr;
    ipv4_addr_t ip_addr;
    ipv4_addr_t broadcast_addr;
    const char* name;
    void(*poll)(struct net_intf_s* intf);
    void(*send)(struct net_intf_s* intf, const void* dst, uint16_t ethertype, net_buf_t* buf);
    void(*dev_send)(net_buf_t* buf);
} net_intf_t;

extern net_intf_t* net_intf_list;

net_intf_t* net_intf_create();
void net_intf_add(net_intf_t* intf);