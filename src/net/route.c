#include "route.h"

#include <sys/kernel_mem.h>
#include <lib/string.h>
#include <lib/kprintf.h>

static net_route_t* route_list = 0;

const net_route_t* net_find_route(const ipv4_addr_t* dst) {
    for (net_route_t* route = route_list; route->next; route = route->next) {
        if ((dst->bits & route->mask.bits) == route->dst.bits) {
            return route;
        }
    }

    return 0;
}

void net_add_route(const ipv4_addr_t* dst, const ipv4_addr_t* mask, const ipv4_addr_t* gateway, net_intf_t* intf) {
    net_route_t* route = pfa_request_page(&pfa);
    memset(route, 0, sizeof(net_route_t));
    route->dst = *dst;
    route->mask = *mask;
    if (gateway) {
        route->gateway = *gateway;
    }

    route->intf = intf;

    if (!route_list) {
        route_list = route;
        route_list->next = 0;
    } else {
        net_route_t* prev;
        for (prev = route_list; prev->next; prev = prev->next) {
            if (prev->mask.bits > mask->bits) {
                break;
            }
        }

        if (prev->next) {
            prev->next->prev = route;
        }

        route->next = prev->next;
        route->prev = prev;
        prev->next = route;
    }
}

const ipv4_addr_t* net_next_addr(const net_route_t* route, const ipv4_addr_t* dst) {
    return route->gateway.bits ? &route->gateway : dst;
}

void net_route_table_dump() {
    puts("Destination      Netmask          Gateway          Interface");

    if (!route_list) {
        return;
    }

    for (net_route_t* route = route_list; route; route = route->next) {
        char dst_str[16];
        char mask_str[16];
        char gateway_str[16];

        memset(dst_str, ' ', 16);
        memset(mask_str, ' ', 16);
        memset(gateway_str, ' ', 16);

        *ip4toa(&route->dst, dst_str) = ' ';
        *ip4toa(&route->mask, mask_str) = ' ';
        if (route->gateway.bits) {
            *ip4toa(&route->gateway, gateway_str) = ' ';
        } else {
            memcpy(gateway_str, "On-link", 7);
        }

        dst_str[15] = 0;
        mask_str[15] = 0;
        gateway_str[15] = 0;

        kprintf("%s  %s  %s  %s\n", dst_str, mask_str, gateway_str, route->intf->name);
    }
}