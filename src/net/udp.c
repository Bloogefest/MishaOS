#include "udp.h"

#include "dhcp.h"
#include "ipv4.h"
#include "net.h"
#include "in.h"
#include "port.h"
#include "dns.h"
#include "ntp.h"
#include "../kprintf.h"
#include "../stdlib.h"

void udp_recv(net_intf_t* intf, const ipv4_header_t* ip_header, net_buf_t* packet) {
    udp_dump(packet);

    if (packet->start + sizeof(udp_header_t) > packet->end) {
        return;
    }

    const udp_header_t* header = (const udp_header_t*) packet->start;
    uint16_t src_port = net_swap16(header->src_port);
    packet->start += sizeof(udp_header_t);

    switch (src_port) {
        case PORT_DNS:
            dns_recv(intf, packet);
            break;

        case PORT_BOOTP_SERVER:
            dhcp_recv(intf, packet);
            break;

        case PORT_NTP:
            ntp_recv(intf, packet);
            break;
    }
}

void udp_send(const ipv4_addr_t* dst_addr, uint32_t dst_port, uint32_t src_port, net_buf_t* packet) {
    packet->start -= sizeof(udp_header_t);
    udp_header_t* header = (udp_header_t*) packet->start;
    header->src_port = net_swap16(src_port);
    header->dst_port = net_swap16(dst_port);
    header->len = net_swap16(packet->end - packet->start);
    header->checksum = 0;

    udp_dump(packet);

    ipv4_send(dst_addr, IP_PROTOCOL_UDP, packet);
}

void udp_intf_send(net_intf_t* intf, const ipv4_addr_t* dst_addr, uint32_t dst_port, uint32_t src_port, net_buf_t* packet) {
    packet->start -= sizeof(udp_header_t);
    udp_header_t* header = (udp_header_t*) packet->start;
    header->src_port = net_swap16(src_port);
    header->dst_port = net_swap16(dst_port);
    header->len = net_swap16(packet->end - packet->start);
    header->checksum = 0;

    udp_dump(packet);

    ipv4_intf_send(intf, dst_addr, dst_addr, IP_PROTOCOL_UDP, packet);
}

void udp_dump(const net_buf_t* packet) {
#ifndef NET_DEBUG
    return;
#endif

    if (packet->start + sizeof(udp_header_t) > packet->end) {
        return;
    }

    const udp_header_t* header = (const udp_header_t*) packet->start;
    uint16_t src_port = net_swap16(header->src_port);
    uint16_t dst_port = net_swap16(header->dst_port);
    uint16_t len = net_swap16(header->len);
    uint16_t checksum = net_swap16(header->checksum);

    kprintf("   UDP: src=%d dst=%d len=%d checksum=%d\n", src_port, dst_port, len, checksum);
}