#include "net.h"

#include <net/arp.h>
#include <net/loopback.h>
#include <net/dhcp.h>
#include <net/tcp.h>

uint8_t net_trace;

void(*net_post_init)();

void net_init() {
    loopback_init();
    arp_init();
    tcp_init();

    for (net_intf_t* intf = net_intf_list; intf; intf = intf->next) {
        if (!intf->ip_addr.bits) {
            dhcp_discover(intf);
        }
    }
}

void net_poll() {
    for (net_intf_t* intf = net_intf_list; intf; intf = intf->next) {
        intf->poll(intf);
    }

    tcp_poll();
}