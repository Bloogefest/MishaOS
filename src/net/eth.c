#include "eth.h"

#include <net/net.h>
#include <net/arp.h>
#include <net/ipv4.h>
#include <net/in.h>
#include <lib/kprintf.h>
#include <lib/stdlib.h>

static uint8_t eth_decode(eth_packet_t* eth, net_buf_t* packet) {
    if (packet->start + sizeof(eth_header_t) > packet->end) {
        return 0;
    }

    uint8_t* data = packet->start;
    eth_header_t* header = (eth_header_t*) data;
    eth->header = header;

    uint16_t n = net_swap16(header->ethertype);
    if (n <= 1500 && packet->start + 22 <= packet->end) {
        uint8_t dsap = data[14];
        uint8_t ssap = data[15];

        if (dsap != 0xAA || ssap != 0xAA) {
            return 0;
        }

        eth->ethertype = (data[20] << 8) | data[21];
        eth->header_len = 22;
    } else {
        eth->ethertype = n;
        eth->header_len = sizeof(eth_header_t);
    }

    return 1;
}

void eth_recv(net_intf_t* intf, net_buf_t* packet) {
    eth_dump(packet);

    eth_packet_t eth;
    if (!eth_decode(&eth, packet)) {
        return;
    }

    packet->start += eth.header_len;

    switch (eth.ethertype) {
        case ET_ARP:
            arp_recv(intf, packet);
            break;

        case ET_IPV4:
            ipv4_recv(intf, packet);
            break;

        case ET_IPV6:
            puts("recv: IPv6");
            break;
    }
}

void eth_intf_send(net_intf_t* intf, const void* dst, uint16_t ethertype, net_buf_t* packet) {
    const eth_addr_t* dst_eth_addr = 0;

    switch (ethertype) {
        case ET_ARP:
            dst_eth_addr = (const eth_addr_t*) dst;
            break;

        case ET_IPV4: {
            const ipv4_addr_t* dst_ip4_addr = (const ipv4_addr_t*) dst;
            if (dst_ip4_addr->bits == ipv4_broadcast_addr.bits || dst_ip4_addr->bits == intf->broadcast_addr.bits) {
                dst_eth_addr = &eth_broadcast_addr;
            }  else {
                dst_eth_addr = arp_lookup_eth_addr(dst_ip4_addr);
                if (!dst_eth_addr) {
                    arp_request(intf, dst_ip4_addr, ethertype, packet);
                    return;
                }
            }

            break;
        }

        case ET_IPV6:
            break;
    }

    if (!dst_eth_addr) {
        puts("Dropped packet");
        return;
    }

    packet->start -= sizeof(eth_header_t);

    eth_header_t* header = (eth_header_t*) packet->start;
    header->dst = *dst_eth_addr;
    header->src = intf->eth_addr;
    header->ethertype = net_swap16(ethertype);

    eth_dump(packet);
    intf->dev_send(packet);
}

void eth_dump(net_buf_t* packet) {
#ifndef NET_DEBUG
        return;
#endif

    eth_packet_t eth;
    if (eth_decode(&eth, packet)) {
        char str[18];
        uint32_t len = packet->end - packet->start - eth.header_len;
        ethtoa(&eth.header->dst, str);
        kprintf("   ETH: dst=%s", str);
        ethtoa(&eth.header->src, str);
        kprintf(" src=%s ethertype=0x04%x len=%ld\n",
                str, eth.ethertype, len);
    }
}