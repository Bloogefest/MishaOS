#include "route.h"

#include "../gpd.h"
#include "../string.h"
#include "../terminal.h"

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
    terminal_putstring("Destination     Netmask         Gateway         Interface\n");

    if (!route_list) {
        return;
    }

    for (net_route_t* route = route_list; route; route = route->next) {
        char str[16];
        ip4toa(&route->dst, str);
        terminal_putstring(str);
        terminal_set_column(16);
        ip4toa(&route->mask, str);
        terminal_putstring(str);
        terminal_set_column(32);
        if (route->gateway.bits) {
            ip4toa(&route->gateway, str);
            terminal_putstring(str);
        } else {
            terminal_putstring("On-link");
        }


        terminal_set_column(48);
        terminal_putstring(route->intf->name);
        terminal_putchar('\n');
    }
}