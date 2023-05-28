#include "dns.h"

#include "net.h"
#include "buf.h"
#include "port.h"
#include "in.h"
#include "udp.h"
#include "../terminal.h"
#include "../stdlib.h"
#include "../string.h"

typedef struct dns_header_s {
    uint16_t id;
    uint16_t flags;
    uint16_t question_count;
    uint16_t answer_count;
    uint16_t authority_count;
    uint16_t additional_count;
} __attribute__((packed)) dns_header_t;

ipv4_addr_t dns_server;

void dns_recv(net_intf_t* intf, const net_buf_t* buf) {
    dns_dump(buf);
    // TODO: Process response
}

void dns_query_host(const char* host, uint32_t id) {
    if (dns_server.bits == ipv4_null_addr.bits) {
        return;
    }

    net_buf_t* packet = net_alloc_buf();
    dns_header_t* header = (dns_header_t*) packet->start;
    header->id = net_swap16(id);
    header->flags = net_swap16(0x0100);
    header->question_count = net_swap16(1);
    header->answer_count = 0;
    header->authority_count = 0;
    header->additional_count = 0;

    uint8_t* q = packet->start + sizeof(dns_header_t);
    uint32_t host_len = strlen(host);
    if (host_len >= 256) {
        return;
    }

    uint8_t* label_head = q++;
    const char* p = host;
    for (;;) {
        char c = *p++;
        if (c == '.' || c == 0) {
            uint32_t label_len = q - label_head - 1;
            *label_head = label_len;
            label_head = q;
        }

        *q++ = c;

        if (!c) {
            break;
        }
    }

    *(uint16_t*) q = net_swap16(1);
    q += sizeof(uint16_t);
    *(uint16_t*) q = net_swap16(1);
    q += sizeof(uint16_t);

    packet->end = q;
    uint32_t src_port = net_ephemeral_port();

    dns_dump(packet);

    udp_send(&dns_server, PORT_DNS, src_port, packet);
}

static const uint8_t* dns_print_host(const net_buf_t* packet, const uint8_t* p, uint8_t first) {
    for (;;) {
        uint8_t count = *p++;
        if (count >= 64) {
            uint8_t n = *p++;
            uint32_t offset = ((count & 0x3F) << 6) | n;

            dns_print_host(packet, packet->start + offset, first);
            return p;
        } else if (count > 0) {
            if (!first) {
                terminal_putchar('.');
            }

            char buf[64];
            memcpy(buf, (void*) p, count);
            buf[count] = 0;
            terminal_putstring(buf);

            p += count;
            first = 0;
        } else {
            return p;
        }
    }
}

static const uint8_t* dns_print_query(const net_buf_t* packet, const uint8_t* p) {
    terminal_putstring("    Query: ");
    p = dns_print_host(packet, p, 1);

    uint16_t query_type = (p[0] << 8) | p[1];
    uint16_t query_class = (p[2] << 8) | p[3];
    p += 4;

    char str[20];
    terminal_putstring(" type=");
    itoa(query_type, str, 10);
    terminal_putstring(str);
    terminal_putstring(" class=");
    itoa(query_class, str, 10);
    terminal_putchar('\n');

    return p;
}

static const uint8_t* dns_print_rr(const char* header, const net_buf_t* packet, const uint8_t* p) {
    terminal_putstring("    ");
    terminal_putstring(header);
    terminal_putstring(": ");
    p = dns_print_host(packet, p, 1);

    uint16_t query_type = (p[0] << 8) | p[1];
    uint16_t query_class = (p[2] << 8) | p[3];
    uint32_t ttl = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
    uint16_t data_len = (p[8] << 8) | p[9];
    p += 10;

    const uint8_t* data = p;

    char str[20];
    terminal_putstring(" type=");
    itoa(query_type, str, 10);
    terminal_putstring(str);
    terminal_putstring(" class=");
    itoa(query_class, str, 10);
    terminal_putstring(str);
    terminal_putstring(" ttl=");
    itoa(ttl, str, 10);
    terminal_putstring(str);
    terminal_putstring(" dataLen=");
    itoa(data_len, str, 10);
    terminal_putstring(str);
    terminal_putchar(' ');

    if (query_type == 1 && data_len == 4) {
        const ipv4_addr_t* addr = (const ipv4_addr_t*) data;
        ip4toa(addr, str);
        terminal_putstring(str);
    } else if (query_type == 2) {
        dns_print_host(packet, data, 1);
    }

    terminal_putchar('\n');

    return p + data_len;
}

void dns_dump(const net_buf_t* packet) {
#ifndef NET_DEBUG
    return;
#endif

    const dns_header_t* header = (const dns_header_t*) packet->start;
    uint16_t id = net_swap16(header->id);
    uint16_t flags = net_swap16(header->flags);
    uint16_t question_count = net_swap16(header->question_count);
    uint16_t answer_count = net_swap16(header->answer_count);
    uint16_t authority_count = net_swap16(header->authority_count);
    uint16_t additional_count = net_swap16(header->additional_count);

    char str[20];
    terminal_putstring("   DNS: id=");
    itoa(id, str, 10);
    terminal_putstring(str);
    terminal_putstring(" flags=0x");
    itoa(flags, str, 16);
    terminal_putstring(str);
    terminal_putstring(" questions=");
    itoa(question_count, str, 10);
    terminal_putstring(str);
    terminal_putstring(" answers=");
    itoa(answer_count, str, 10);
    terminal_putstring(str);
    terminal_putstring(" authorities=");
    itoa(authority_count, str, 10);
    terminal_putstring(str);
    terminal_putstring(" additional=");
    itoa(additional_count, str, 10);
    terminal_putstring(str);
    terminal_putchar('\n');

    const uint8_t* p = packet->start + sizeof(dns_header_t);
    for (uint32_t i = 0; i < question_count; ++i) {
        p = dns_print_query(packet, p);
    }

    for (uint32_t i = 0; i < answer_count; ++i) {
        p = dns_print_rr("Ans", packet, p);
    }

    for (uint32_t i = 0; i < authority_count; ++i) {
        p = dns_print_rr("Auth", packet, p);
    }

    for (uint32_t i = 0; i < additional_count; ++i) {
        dns_print_rr("Add", packet, p);
    }
}