#include "net.h"

#include "arp.h"
#include "loopback.h"
#include "dhcp.h"
#include "tcp.h"

uint8_t net_trace;

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