#include "icmp.h"

#include "net.h"
#include "checksum.h"
#include "../string.h"
#include "../stdlib.h"
#include "../terminal.h"

static void icmp_dump(const net_buf_t* packet) {
#ifndef NET_DEBUG
    return;
#endif

    if (packet->start + 8 > packet->end) {
        return;
    }

    uint8_t* data = packet->start;
    uint8_t type = data[0];
    uint8_t code = data[1];
    uint16_t checksum = (data[2] << 8) | data[3];
    uint16_t id = (data[4] << 8) | data[5];
    uint16_t sequence = (data[6] << 8) | data[7];

    uint32_t actual_checksum = net_checksum(packet->start, packet->end);

    char str[20];
    terminal_putstring("   ICMP: type=");
    itoa(type, str, 10);
    terminal_putstring(str);
    terminal_putstring(" code=");
    itoa(code, str, 10);
    terminal_putstring(str);
    terminal_putstring(" id=");
    itoa(id, str, 10);
    terminal_putstring(str);
    terminal_putstring(" sequence=");
    itoa(sequence, str, 10);
    terminal_putstring(str);
    terminal_putstring(" len=");
    itoa(packet->end - packet->start, str, 10);
    terminal_putstring(str);
    terminal_putstring(" checksum=");
    itoa(checksum, str, 10);
    terminal_putstring(str);
    terminal_putchar(actual_checksum ? '!' : ' ');
    terminal_putchar('\n');
}

static void icmp_echo_reply(const ipv4_addr_t* addr, uint16_t id, uint16_t sequence, const uint8_t* data, const uint8_t* end) {
    uint32_t len = end - data;
    net_buf_t* packet = net_alloc_buf();
    uint8_t* ptr = packet->start;
    ptr[0] = ICMP_TYPE_ECHO_REPLY;
    ptr[1] = 0;
    ptr[2] = 0;
    ptr[3] = 0;
    ptr[4] = (id >> 8) & 0xFF;
    ptr[5] = id & 0xFF;
    ptr[6] = (sequence >> 8) & 0xFF;
    ptr[7] = sequence & 0xFF;
    memcpy(ptr + 8, data, len);
    packet->end += 8 + len;

    uint32_t checksum = net_checksum(packet->start, packet->end);
    ptr[2] = (checksum >> 8) & 0xFF;
    ptr[3] = checksum & 0xFF;

    icmp_dump(packet);
    ipv4_send(addr, IP_PROTOCOL_ICMP, packet);
}

void icmp_recv(net_intf_t* intf, const ipv4_header_t* header, net_buf_t* packet) {
    icmp_dump(packet);

    if (packet->start + 8 > packet->end) {
        return;
    }

    const uint8_t* data = packet->start;
    uint8_t type = data[0];
    uint16_t id = (data[4] << 8) | data[5];
    uint16_t sequence = (data[6] << 8) | data[7];

    switch (type) {
        case ICMP_TYPE_ECHO_REQUEST: {
            char str[16];
            ip4toa(&header->src, str);
            terminal_putstring("ICMP: Echo request from ");
            terminal_putstring(str);
            terminal_putchar('\n');
            icmp_echo_reply(&header->src, id, sequence, data + 8, packet->end);
            break;
        }

        case ICMP_TYPE_ECHO_REPLY: {
            char str[16];
            ip4toa(&header->src, str);
            terminal_putstring("ICMP: Echo reply from ");
            terminal_putstring(str);
            terminal_putchar('\n');
            break;
        }
    }
}

void icmp_echo_request(const ipv4_addr_t* addr, uint16_t id, uint16_t seq, const uint8_t* data, const uint8_t* end) {
    uint32_t len = end - data;
    net_buf_t* packet = net_alloc_buf();
    uint8_t* ptr = packet->start;
    ptr[0] = ICMP_TYPE_ECHO_REQUEST;
    ptr[1] = 0;
    ptr[2] = 0;
    ptr[3] = 0;
    ptr[4] = (id >> 8) & 0xFF;
    ptr[5] = id & 0xFF;
    ptr[6] = (seq >> 8) & 0xFF;
    ptr[7] = seq & 0xFF;
    memcpy(ptr + 8, data, len);
    packet->end += 8 + len;

    uint32_t checksum = net_checksum(packet->start, packet->end);
    ptr[2] = (checksum >> 8) & 0xFF;
    ptr[3] = checksum & 0xFF;

    icmp_dump(packet);
    ipv4_send(addr, IP_PROTOCOL_ICMP, packet);
}