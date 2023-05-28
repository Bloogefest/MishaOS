#include "dhcp.h"

#include "net.h"
#include "dns.h"
#include "buf.h"
#include "route.h"
#include "in.h"
#include "udp.h"
#include "port.h"
#include "../terminal.h"
#include "../string.h"
#include "../stdlib.h"

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
                    terminal_putstring("[DHCP] unknown option (");
                    char num[20];
                    itoa(type, num, 10);
                    terminal_putstring(num);
                    terminal_putstring(")\n");
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
    terminal_putstring("DHCP requested lease for ");
    terminal_putstring(str);
    terminal_putchar('\n');

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
            terminal_putstring("DHCP offer received for ");
            terminal_putstring(ip_str);
            terminal_putchar('\n');
            dhcp_request(intf, header, &opt);
            break;
        }

        case DHCP_ACK: {
            terminal_putstring("DHCP ack received for ");
            terminal_putstring(ip_str);
            terminal_putchar('\n');
            dhcp_ack(intf, header, &opt);
            break;
        }

        case DHCP_NAK: {
            terminal_putstring("DHCP nak received for ");
            terminal_putstring(ip_str);
            terminal_putchar('\n');
            break;
        }

        default: {
            terminal_putstring("DHCP message unhandled\n");
            break;
        }
    }
}

void dhcp_discover(net_intf_t* intf) {
    terminal_putstring("DHCP discovery for ");
    terminal_putstring(intf->name);
    terminal_putstring(":\n");

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
    terminal_putstring("   DHCP: opcode=");
    itoa(header->opcode, str, 10);
    terminal_putstring(str);
    terminal_putstring(" htype=");
    itoa(header->htype, str, 10);
    terminal_putstring(str);
    terminal_putstring(" hlen=");
    itoa(header->hlen, str, 10);
    terminal_putstring(str);
    terminal_putstring(" hopCount=");
    itoa(header->hop_count, str, 10);
    terminal_putstring(str);
    terminal_putstring(" xid=");
    itoa(net_swap32(header->xid), str, 10);
    terminal_putstring(str);
    terminal_putstring(" secs=");
    itoa(net_swap32(header->sec_count), str, 10);
    terminal_putstring(str);
    terminal_putstring("\n   DHCP: flags=");
    itoa(net_swap16(header->flags), str, 10);
    terminal_putstring(str);
    terminal_putstring(" len=");
    itoa(packet->end - packet->start, str, 10);
    terminal_putstring(str);
    terminal_putstring(" client=");
    ip4toa(&header->client_ip_addr, str);
    terminal_putstring(str);
    terminal_putstring(" your=");
    ip4toa(&header->your_ip_addr, str);
    terminal_putstring(str);
    terminal_putstring(" server=");
    ip4toa(&header->server_ip_addr, str);
    terminal_putstring(str);
    terminal_putstring(" gateway=");
    ip4toa(&header->gateway_ip_addr, str);
    terminal_putstring(str);
    terminal_putstring(" eth=");
    ethtoa(&header->client_eth_addr, str);
    terminal_putstring(str);
    terminal_putstring("\n   DHCP: serverName=");
    terminal_putstring(header->server_name);
    terminal_putstring(" bootFilename=");
    terminal_putstring(header->boot_filename);
    terminal_putchar('\n');

    dhcp_options_t opt;
    if (!dhcp_parse_options(&opt, packet)) {
        return;
    }

    if (opt.message_type) {
        terminal_putstring("   DHCP: message type: ");
        itoa(opt.message_type, str, 10);
        terminal_putstring(str);
        terminal_putchar('\n');
    }

    if (opt.subnet_mask) {
        terminal_putstring("   DHCP: subnetMask: ");
        ip4toa(opt.subnet_mask, str);
        terminal_putstring(str);
        terminal_putchar('\n');
    }

    for (const ipv4_addr_t* addr = opt.router_list; addr != opt.router_end; ++addr) {
        terminal_putstring("   DHCP: router: ");
        ip4toa(addr, str);
        terminal_putstring(str);
        terminal_putchar('\n');
    }

    if (opt.requested_ip_addr) {
        terminal_putstring("   DHCP: requested ip: ");
        ip4toa(opt.requested_ip_addr, str);
        terminal_putstring(str);
        terminal_putchar('\n');
    }

    if (opt.server_id) {
        terminal_putstring("   DHCP: server id: ");
        ip4toa(opt.server_id, str);
        terminal_putstring(str);
        terminal_putchar('\n');
    }

    for (const uint8_t* p = opt.parameter_list; p != opt.parameter_end; ++p) {
        terminal_putstring("   DHCP: parameter request: ");
        itoa(*p, str, 10);
        terminal_putstring(str);
        terminal_putchar('\n');
    }
}