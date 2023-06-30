#include "arp.h"

#include "in.h"
#include "eth.h"
#include "../kprintf.h"
#include "../stdlib.h"
#include "../string.h"

#define ARP_HTYPE_ETH 0x01

#define ARP_OP_REQUEST 0x01
#define ARP_OP_REPLY 0x02

#define ARP_CACHE_SIZE 16

typedef struct arp_entry_s {
    eth_addr_t ha;
    ipv4_addr_t pa;
    net_intf_t* intf;
    uint16_t ethertype;
    net_buf_t* packet;
} arp_entry_t;

static arp_entry_t arp_cache[ARP_CACHE_SIZE];

static void arp_dump(const net_buf_t* packet) {
#ifndef NET_DEBUG
    return;
#endif

    if (packet->start + sizeof(arp_header_t) > packet->end) {
        return;
    }

    const uint8_t* data = packet->start;
    const arp_header_t* header = (const arp_header_t*) data;

    uint16_t htype = net_swap16(header->htype);
    uint16_t ptype = net_swap16(header->ptype);
    uint8_t hlen = header->hlen;
    uint8_t plen = header->plen;
    uint16_t op = net_swap16(header->op);

    kprintf("   ARP: htype=0x%x ptype=0x%x hlen=%d plen=%d op=%d", htype, ptype, hlen, plen, op);
    if (htype == ARP_HTYPE_ETH && ptype == ET_IPV4 && packet->start + 28 <= packet->end) {
        const eth_addr_t* sha = (const eth_addr_t*) (data + 8);
        const ipv4_addr_t* spa = (const ipv4_addr_t*) (data + 14);
        const eth_addr_t* tha = (const eth_addr_t*) (data + 18);
        const ipv4_addr_t* tpa = (const ipv4_addr_t*) (data + 24);

        char str[20];

        ethtoa(sha, str);
        kprintf(" [sha=%s", str);
        ip4toa(spa, str);
        kprintf(" spa=%s]", str);
        ethtoa(tha, str);
        kprintf(" [tha=%s", str);
        ip4toa(tpa, str);
        kprintf(" tpa=%s]", str);
    }

    putchar('\n');
}

static void arp_send(net_intf_t* intf, uint32_t op, const eth_addr_t* tha, const ipv4_addr_t* tpa) {
    net_buf_t* packet = net_alloc_buf();
    uint8_t* data = packet->start;
    arp_header_t* header = (arp_header_t*) data;
    header->htype = net_swap16(ARP_HTYPE_ETH);
    header->ptype = net_swap16(ET_IPV4);
    header->hlen = sizeof(eth_addr_t);
    header->plen = sizeof(ipv4_addr_t);
    header->op = net_swap16(op);
    *(eth_addr_t*)(data + 8) = intf->eth_addr;
    *(ipv4_addr_t*)(data + 14) = intf->ip_addr;
    if (op == ARP_OP_REQUEST) {
        *(eth_addr_t*)(data + 18) = eth_null_addr;
    } else {
        *(eth_addr_t*)(data + 18) = *tha;
    }

    *(ipv4_addr_t*)(data + 24) = *tpa;
    packet->end += 28;

    arp_dump(packet);
    intf->send(intf, tha, ET_ARP, packet);
}

static arp_entry_t* arp_lookup(const ipv4_addr_t* pa) {
    arp_entry_t* entry = arp_cache;
    arp_entry_t* end = entry + ARP_CACHE_SIZE;
    for (; entry != end; ++entry) {
        if (entry->pa.bits == pa->bits) {
            return entry;
        }
    }

    return 0;
}

static arp_entry_t* arp_add(const eth_addr_t* ha, const ipv4_addr_t* pa) {
    arp_entry_t* entry = arp_cache;
    arp_entry_t* end = entry + ARP_CACHE_SIZE;
    for (; entry != end; ++entry) {
        if (!entry->pa.bits) {
            break;
        }
    }

    if (entry == end) {
        memcpy(arp_cache, arp_cache + 1, sizeof(arp_entry_t) * (ARP_CACHE_SIZE - 1));
        entry = end - 1;
    }

    entry->ha = *ha;
    entry->pa = *pa;
    return entry;
}

void arp_init() {
    arp_entry_t* end = arp_cache + ARP_CACHE_SIZE;
    for (arp_entry_t* entry = arp_cache; entry != end; ++entry) {
        memset(&entry->ha, 0, sizeof(eth_addr_t));
        memset(&entry->pa, 0, sizeof(ipv4_addr_t));
    }
}

const eth_addr_t* arp_lookup_eth_addr(const ipv4_addr_t* pa) {
    arp_entry_t* entry = arp_lookup(pa);
    if (entry) {
        return &entry->ha;
    }

    return 0;
}

void arp_request(net_intf_t* intf, const ipv4_addr_t* tpa, uint16_t ethertype, net_buf_t* packet) {
    arp_entry_t* entry = arp_lookup(tpa);
    if (!entry) {
        entry = arp_add(&eth_null_addr, tpa);
    }

    if (entry->packet) {
        net_free_buf(entry->packet);
    }

    entry->intf = intf;
    entry->ethertype = ethertype;
    entry->packet = packet;
    arp_send(intf, ARP_OP_REQUEST, &eth_broadcast_addr, tpa);
}

void arp_reply(net_intf_t* intf, const eth_addr_t* ha, const ipv4_addr_t* pa) {
    arp_send(intf, ARP_OP_REPLY, ha, pa);
}

void arp_recv(net_intf_t* intf, net_buf_t* packet) {
    arp_dump(packet);
    if (packet->start + sizeof(arp_header_t) > packet->end) {
        return;
    }

    const uint8_t* data = packet->start;
    const arp_header_t* header = (const arp_header_t*) data;

    uint16_t htype = net_swap16(header->htype);
    uint16_t ptype = net_swap16(header->ptype);
    uint16_t op = net_swap16(header->op);
    if (htype != ARP_HTYPE_ETH || ptype != ET_IPV4 || packet->start + 28 > packet->end) {
        return;
    }

    const eth_addr_t* sha = (const eth_addr_t*) (data + 8);
    const ipv4_addr_t* spa = (const ipv4_addr_t*) (data + 14);
    const ipv4_addr_t* tpa = (const ipv4_addr_t*) (data + 24);

    uint8_t merge = 0;
    arp_entry_t* entry = arp_lookup(spa);
    if (entry) {
        entry->ha = *sha;
        merge = 1;

        if (entry->packet) {
            puts("[ARP] Resending packet");
            eth_intf_send(entry->intf, spa, entry->ethertype, entry->packet);
            entry->intf = 0;
            entry->ethertype = 0;
            entry->packet = 0;
        }
    }

    if (tpa->bits == intf->ip_addr.bits) {
        if (!merge) {
            arp_add(sha, spa);
        }

        if (op == ARP_OP_REQUEST) {
            arp_reply(intf, sha, spa);
        }
    }
}