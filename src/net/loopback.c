#include "loopback.h"

#include "arp.h"
#include "eth.h"
#include "ipv4.h"
#include "intf.h"
#include "route.h"

static void loop_poll(net_intf_t* intf) {
    // Nothing to do.
}

static void loop_send(net_intf_t* intf, const void* dst, uint16_t ethertype, net_buf_t* packet) {
    switch (ethertype) {
        case ET_ARP:
            arp_recv(intf, packet);
            break;

        case ET_IPV4:
            ipv4_recv(intf, packet);
            break;

        case ET_IPV6:
            // TODO: IPv6
            break;
    }

    net_free_buf(packet);
}

void loopback_init() {
    ipv4_addr_t ip_addr = {.address = {127, 0, 0, 1}};
    ipv4_addr_t subnet_mask = {.address = {255, 255, 255, 255}};

    net_intf_t* intf = net_intf_create();
    intf->eth_addr = eth_null_addr;
    intf->ip_addr = ip_addr;
    intf->name = "loop";
    intf->poll = loop_poll;
    intf->send = loop_send;
    intf->dev_send = 0;

    net_intf_add(intf);
    net_add_route(&ip_addr, &subnet_mask, 0, intf);
}