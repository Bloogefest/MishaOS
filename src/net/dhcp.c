#include "dhcp.h"

#include <net/net.h>
#include <net/ntp.h>
#include <net/dns.h>
#include <net/buf.h>
#include <net/route.h>
#include <net/in.h>
#include <net/udp.h>
#include <net/port.h>
#include <lib/kprintf.h>
#include <lib/string.h>
#include <lib/stdlib.h>

#define OP_REQUEST 1
#define OP_REPLY 2

#define HTYPE_ETH 1

#define MAGIC_COOKIE 0x63825363

#define OPT_PAD 0
#define OPT_SUBNET_MASK 1
#define OPT_ROUTER 3
#define OPT_DNS 6
#define OPT_REQUESTED_IP_ADDR 50
#define OPT_LEASE_TIME 51
#define OPT_DHCP_MESSAGE_TYPE 53
#define OPT_SERVER_ID 54
#define OPT_PARAMETER_REQUEST 55
#define OPT_END 255

#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_DECLINE 4
#define DHCP_ACK 5
#define DHCP_NAK 6
#define DHCP_RELEASE 7
#define DHCP_INFORM 8

typedef struct dhcp_header_s {
    uint8_t opcode;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hop_count;
    uint32_t xid;
    uint16_t sec_count;
    uint16_t flags;
    ipv4_addr_t client_ip_addr;
    ipv4_addr_t your_ip_addr;
    ipv4_addr_t server_ip_addr;
    ipv4_addr_t gateway_ip_addr;
    eth_addr_t client_eth_addr;
    uint8_t reserved[10];
    char server_name[64];
    char boot_filename[128];
} __attribute__((packed)) dhcp_header_t;

typedef struct dhcp_options_s {
    const ipv4_addr_t* subnet_mask;
    const ipv4_addr_t* router_list;
    const ipv4_addr_t* router_end;
    const ipv4_addr_t* dns_list;
    const ipv4_addr_t* dns_end;
    const ipv4_addr_t* requested_ip_addr;
    uint32_t lease_time;
    uint32_t message_type;
    const ipv4_addr_t* server_id;
    const uint8_t* parameter_list;
    const uint8_t* parameter_end;
} dhcp_options_t;

static uint8_t dhcp_parse_options(dhcp_options_t* opt, const net_buf_t* packet) {
    const uint8_t* ptr = packet->start + sizeof(dhcp_header_t);
    const uint8_t* end = packet->end;

    if (ptr + 4 > end) {
        return 0;
    }

    uint32_t magic_cookie = net_swap32(*(uint32_t*) ptr);
    ptr += 4;

    if (magic_cookie != MAGIC_COOKIE) {
        return 0;
    }

    memset(opt, 0, sizeof(*opt));
    while (ptr < end) {
        uint8_t type = *ptr++;
        if (type == OPT_PAD) {
            continue;
        } else if (type == OPT_END) {
            break;
        } else {
            uint8_t opt_len = *ptr++;

            const uint8_t* next = ptr + opt_len;
            if (next > end) {
                return 0;
            }

            switch (type) {
                case OPT_SUBNET_MASK:
                    opt->subnet_mask = (const ipv4_addr_t*) ptr;
                    break;

                case OPT_ROUTER:
                    opt->router_list = (const ipv4_addr_t*) ptr;
                    opt->router_end = (const ipv4_addr_t*) next;
                    break;

                case OPT_DNS:
                    opt->dns_list = (const ipv4_addr_t*) ptr;
                    opt->dns_end = (const ipv4_addr_t*) next;
                    break;

                case OPT_LEASE_TIME:
                    opt->lease_time = net_swap32(*(uint32_t*) ptr);
                    break;

                case OPT_DHCP_MESSAGE_TYPE:
                    opt->message_type = *ptr;
                    break;

                case OPT_SERVER_ID:
                    opt->server_id = (const ipv4_addr_t*) ptr;
                    break;

                case OPT_PARAMETER_REQUEST:
                    opt->parameter_list = ptr;
                    opt->parameter_end = next;
                    break;

                default:
                    kprintf("[DHCP] unknown option (%d)\n", type);
                    break;
            }

            ptr = next;
        }
    }

    return 1;
}

static uint8_t* dhcp_build_header(net_buf_t* packet, uint32_t xid, const eth_addr_t* client_eth_addr, uint8_t message_type) {
    dhcp_header_t* header = (dhcp_header_t*) packet->start;
    memset(header, 0, sizeof(dhcp_header_t));
    header->opcode = OP_REQUEST;
    header->htype = HTYPE_ETH;
    header->hlen = sizeof(eth_addr_t);
    header->hop_count = 0;
    header->xid = net_swap32(xid);
    header->sec_count = 0;
    header->flags = 0;
    header->client_eth_addr = *client_eth_addr;

    uint8_t* ptr = packet->start + sizeof(dhcp_header_t);
    *(uint32_t*) ptr = net_swap32(MAGIC_COOKIE);
    ptr += 4;

    *ptr++ = OPT_DHCP_MESSAGE_TYPE;
    *ptr++ = 1;
    *ptr++ = message_type;

    return ptr;
}

static void dhcp_request(net_intf_t* intf, const dhcp_header_t* header, const dhcp_options_t* opt) {
    uint32_t xid = net_swap32(header->xid);
    const ipv4_addr_t* requested_ip_addr = &header->your_ip_addr;
    const ipv4_addr_t* server_id = opt->server_id;

    char str[16];
    ip4toa(requested_ip_addr, str);
    kprintf("DHCP requested lease for %s\n", str);

    net_buf_t* packet = net_alloc_buf();
    uint8_t* ptr = dhcp_build_header(packet, xid, &intf->eth_addr, DHCP_REQUEST);

    *ptr++ = OPT_SERVER_ID;
    *ptr++ = sizeof(ipv4_addr_t);
    *(ipv4_addr_t*) ptr = *server_id;
    ptr += sizeof(ipv4_addr_t);

    *ptr++ = OPT_REQUESTED_IP_ADDR;
    *ptr++ = sizeof(ipv4_addr_t);
    *(ipv4_addr_t*) ptr = *requested_ip_addr;
    ptr += sizeof(ipv4_addr_t);

    *ptr++ = OPT_PARAMETER_REQUEST;
    *ptr++ = 3;
    *ptr++ = OPT_SUBNET_MASK;
    *ptr++ = OPT_ROUTER;
    *ptr++ = OPT_DNS;

    *ptr++ = OPT_END;

    packet->end = ptr;

    dhcp_dump(packet);
    udp_intf_send(intf, &ipv4_broadcast_addr, PORT_BOOTP_SERVER, PORT_BOOTP_CLIENT, packet);
}

static void dhcp_ack(net_intf_t* intf, const dhcp_header_t* header, const dhcp_options_t* opt) {
    intf->ip_addr = header->your_ip_addr;

    if (opt->router_list) {
        net_add_route(&ipv4_null_addr, &ipv4_null_addr, opt->router_list, intf);
    }

    if (opt->subnet_mask) {
        ipv4_addr_t subnet_addr;
        subnet_addr.bits = intf->ip_addr.bits & opt->subnet_mask->bits;
        net_add_route(&subnet_addr, opt->subnet_mask, 0, intf);
    }

    ipv4_addr_t host_mask = {.address = {0xFF, 0xFF, 0xFF, 0xFF}};
    net_add_route(&intf->ip_addr, &host_mask, 0, intf);

    if (opt->subnet_mask) {
        intf->broadcast_addr.bits = intf->ip_addr.bits | ~opt->subnet_mask->bits;
    }

    if (opt->dns_list) {
        dns_server = *opt->dns_list;
    }

    net_route_table_dump();

    const ipv4_addr_t ntp_server = {{216, 239, 35, 0}};
    ntp_send(&ntp_server);

    net_post_init(intf);
}

void dhcp_recv(net_intf_t* intf, const net_buf_t* packet) {
    dhcp_dump(packet);

    if (packet->start + sizeof(dhcp_header_t) > packet->end) {
        return;
    }

    const dhcp_header_t* header = (const dhcp_header_t*) packet->start;
    if (header->opcode != OP_REPLY || header->htype != HTYPE_ETH || header->hlen != sizeof(eth_addr_t)) {
        return;
    }

    if (memcmp(intf->eth_addr.address, (void*) header->client_eth_addr.address, sizeof(eth_addr_t))) {
        return;
    }

    dhcp_options_t opt;
    if (!dhcp_parse_options(&opt, packet)) {
        return;
    }

    char ip_str[16];
    ip4toa(&header->your_ip_addr, ip_str);

    switch (opt.message_type) {
        case DHCP_OFFER: {
            kprintf("DHCP offer received for %s\n", ip_str);
            dhcp_request(intf, header, &opt);
            break;
        }

        case DHCP_ACK: {
            kprintf("DHCP ack received for %s\n", ip_str);
            dhcp_ack(intf, header, &opt);
            break;
        }

        case DHCP_NAK: {
            kprintf("DHCP nak received for %s\n", ip_str);
            break;
        }

        default: {
            puts("DHCP message unhandled");
            break;
        }
    }
}

void dhcp_discover(net_intf_t* intf) {
    kprintf("DHCP discovery for %s\n", intf->name);

    net_buf_t* packet = net_alloc_buf();

    uint32_t xid = 0;
    uint8_t* ptr = dhcp_build_header(packet, xid, &intf->eth_addr, DHCP_DISCOVER);

    *ptr++ = OPT_PARAMETER_REQUEST;
    *ptr++ = 3;
    *ptr++ = OPT_SUBNET_MASK;
    *ptr++ = OPT_ROUTER;
    *ptr++ = OPT_DNS;
    *ptr++ = OPT_END;
    packet->end = ptr;

    dhcp_dump(packet);

    udp_intf_send(intf, &ipv4_broadcast_addr, PORT_BOOTP_SERVER, PORT_BOOTP_CLIENT, packet);
}

void dhcp_dump(const net_buf_t* packet) {
#ifndef NET_DEBUG
    return;
#endif

    if (packet->start + sizeof(dhcp_header_t) > packet->end) {
        return;
    }

    const dhcp_header_t* header = (const dhcp_header_t*) packet->start;

    char str[34];
    kprintf("   DHCP: opcode=%d htype=%d hlen=%d hopCount=%d xid=%ld secs=%d\n",
            header->opcode, header->htype, header->hlen, header->hop_count,
            net_swap32(header->xid), net_swap16(header->sec_count));
    ip4toa(&header->client_ip_addr, str);
    kprintf("   DHCP: flags=%d len=%lu client=%s",
            net_swap16(header->flags), packet->end - packet->start, str);
    ip4toa(&header->your_ip_addr, str);
    kprintf(" your=%s", str);
    ip4toa(&header->server_ip_addr, str);
    kprintf(" server=%s", str);
    ip4toa(&header->gateway_ip_addr, str);
    kprintf(" gateway=%s", str);
    ethtoa(&header->client_eth_addr, str);
    kprintf(" eth=%s\n", str);
    kprintf("   DHCP: serverName=%s bootFilename=%s\n",
            header->server_name, header->boot_filename);

    dhcp_options_t opt;
    if (!dhcp_parse_options(&opt, packet)) {
        return;
    }

    if (opt.message_type) {
        kprintf("   DHCP: message type: %ld\n", opt.message_type);
    }

    if (opt.subnet_mask) {
        ip4toa(opt.subnet_mask, str);
        kprintf("   DHCP: subnetMask: %s\n", str);
    }

    for (const ipv4_addr_t* addr = opt.router_list; addr != opt.router_end; ++addr) {
        ip4toa(addr, str);
        kprintf("   DHCP: router: %s\n", str);
    }

    if (opt.requested_ip_addr) {
        ip4toa(opt.requested_ip_addr, str);
        kprintf("   DHCP: requested ip: %s\n", str);
    }

    if (opt.server_id) {
        ip4toa(opt.server_id, str);
        kprintf("   DHCP: server id: %s\n", str);
    }

    for (const uint8_t* p = opt.parameter_list; p != opt.parameter_end; ++p) {
        kprintf("   DHCP: parameter request: %d\n", *p);
    }
}