#include "ipv4.h"

#include "net.h"
#include "in.h"
#include "eth.h"
#include "checksum.h"
#include "route.h"
#include "udp.h"
#include "tcp.h"
#include "icmp.h"
#include "../terminal.h"
#include "../stdlib.h"

void ipv4_recv(net_intf_t* intf, net_buf_t* packet) {
    ipv4_dump(packet);

    if (packet->start + sizeof(ipv4_header_t) > packet->end) {
        return;
    }

    const ipv4_header_t* header = (const ipv4_header_t*) packet->start;

    uint32_t version = (header->ver_ihl >> 4) & 0x0F;
    if (version != 4) {
        return;
    }

    uint16_t fragment = net_swap16(header->offset) & 0x1FFF;
    if (fragment) {
        return;
    }

    uint32_t ihl = (header->ver_ihl) & 0xF;
    uint8_t* ip_end = packet->start + net_swap16(header->len);
    if (ip_end > packet->end) {
        terminal_putstring("   IPv4: packet too long.\n");
        return;
    }

    packet->start += ihl << 2;
    packet->end = ip_end;
    switch (header->protocol) {
        case IP_PROTOCOL_ICMP:
            icmp_recv(intf, header, packet);
            break;

        case IP_PROTOCOL_TCP:
            tcp_recv(intf, header, packet);
            break;

        case IP_PROTOCOL_UDP:
            udp_recv(intf, header, packet);
            break;
    }
}

void ipv4_send(const ipv4_addr_t* dst, uint8_t protocol, net_buf_t* packet) {
    const net_route_t* route = net_find_route(dst);
    if (route) {
        const ipv4_addr_t* next_addr = net_next_addr(route, dst);
        ipv4_intf_send(route->intf, next_addr, dst, protocol, packet);
    }
}

void ipv4_intf_send(net_intf_t* intf, const ipv4_addr_t* next_addr, const ipv4_addr_t* dst, uint8_t protocol, net_buf_t* packet) {
    packet->start -= sizeof(ipv4_header_t);

    ipv4_header_t* header = (ipv4_header_t*) packet->start;
    header->ver_ihl = (4 << 4) | 5;
    header->tos = 0;
    header->len = net_swap16(packet->end - packet->start);
    header->id = 0;
    header->offset = 0;
    header->ttl = 64;
    header->protocol = protocol;
    header->checksum = 0;
    header->src = intf->ip_addr;
    header->dst = *dst;
    header->checksum = net_swap16(net_checksum(packet->start, packet->start + sizeof(ipv4_header_t)));

    ipv4_dump(packet);

    intf->send(intf, next_addr, ET_IPV4, packet);
}

void ipv4_dump(const net_buf_t* packet) {
#ifndef NET_DEBUG
    return;
#endif

    if (packet->start + sizeof(ipv4_header_t) > packet->end) {
        return;
    }

    const ipv4_header_t* header = (const ipv4_header_t*) packet->start;
    uint32_t version = (header->ver_ihl >> 4) & 0xF;
    uint32_t ihl = (header->ver_ihl) & 0xF;
    uint32_t dscp = (header->tos >> 2) & 0x3F;
    uint32_t ecn = header->tos & 0x3;
    uint16_t len = net_swap16(header->len);
    uint16_t id = net_swap16(header->id);
    uint16_t fragment = net_swap16(header->offset) & 0x1FFF;
    uint8_t ttl = header->ttl;
    uint8_t prtocol = header->protocol;
    uint16_t checksum = net_swap16(header->checksum);
    uint32_t actual_checksum = net_checksum(packet->start, packet->start + sizeof(ipv4_header_t));

    char str[20];
    terminal_putstring("   IPv4: version=");
    itoa(version, str, 10);
    terminal_putstring(str);
    terminal_putstring(" ihl=");
    itoa(ihl, str, 10);
    terminal_putstring(str);
    terminal_putstring(" dscp=");
    itoa(dscp, str, 10);
    terminal_putstring(str);
    terminal_putstring(" ecn=");
    itoa(ecn, str, 10);
    terminal_putstring(str);
    terminal_putstring(" len=");
    itoa(len, str, 10);
    terminal_putstring(str);
    terminal_putstring(" id=");
    itoa(id, str, 10);
    terminal_putstring(str);
    terminal_putstring(" fragment=");
    itoa(fragment, str, 10);
    terminal_putstring(str);
    terminal_putstring(" ttl=");
    itoa(ttl, str, 10);
    terminal_putstring(str);
    terminal_putstring(" protocol=");
    itoa(prtocol, str, 10);
    terminal_putstring(str);
    terminal_putstring("\n   IPv4: checksum=");
    itoa(checksum, str, 10);
    terminal_putstring(str);
    terminal_putstring(actual_checksum ? "!" : "");
    terminal_putstring(" dst=");
    ip4toa(&header->dst, str);
    terminal_putstring(str);
    terminal_putstring(" src=");
    ip4toa(&header->src, str);
    terminal_putstring(str);
    terminal_putchar('\n');
}