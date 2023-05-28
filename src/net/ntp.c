#include "ntp.h"

#include "buf.h"
#include "net.h"
#include "port.h"
#include "in.h"
#include "udp.h"
#include "../terminal.h"
#include "../stdlib.h"
#include "../rtc.h"

#define NTP_VERSION 4
#define UNIX_EPOCH 0x83AA7E80

#define MODE_CLIENT 3
#define MODE_SERVER 4

typedef struct ntp_header_s {
    uint8_t mode;
    uint8_t stratum;
    uint8_t poll;
    int8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint64_t ref_timestamp;
    uint64_t orig_timestamp;
    uint64_t recv_timestamp;
    uint64_t send_timestamp;
} __attribute__((packed)) ntp_header_t;

void ntp_recv(net_intf_t* intf, const net_buf_t* packet) {
    ntp_dump(packet);

    if (packet->start + sizeof(ntp_header_t) > packet->end) {
        return;
    }

    ntp_header_t* header = (ntp_header_t*) packet->start;
    abs_time time = (net_swap64(header->send_timestamp) >> 32) - UNIX_EPOCH;
    date_time_t date;
    split_time(&date, time, local_time_zone);

    char str[TIME_STRING_SIZE];
    format_time(str, &date);
    terminal_putstring("Setting time to ");
    terminal_putstring(str);
    terminal_putchar('\n');

    rtc_set_time(&date);
}

void ntp_send(const ipv4_addr_t* dst_addr) {
    net_buf_t* packet = net_alloc_buf();
    ntp_header_t* header = (ntp_header_t*) packet->start;
    header->mode = (NTP_VERSION << 3) | MODE_CLIENT;
    header->stratum = 0;
    header->poll = 4;
    header->precision = -6;
    header->root_delay = net_swap32(1 << 16);
    header->root_dispersion = net_swap32(1 << 16);
    header->ref_id = 0;
    header->ref_timestamp = 0;
    header->orig_timestamp = 0;
    header->recv_timestamp = 0;
    header->send_timestamp = 0;

    packet->end += sizeof(ntp_header_t);
    uint32_t src_port = net_ephemeral_port();

    ntp_dump(packet);

    udp_send(dst_addr, PORT_NTP, src_port, packet);
}

void ntp_dump(const net_buf_t* packet) {
#ifndef NET_DEBUG
    return;
#endif

    if (packet->start + sizeof(ntp_header_t) > packet->end) {
        return;
    }

    ntp_header_t* header = (ntp_header_t*) packet->start;
    abs_time time = (net_swap64(header->send_timestamp) >> 32) - UNIX_EPOCH;

    char str[20];
    terminal_putstring("   NTP: mode=");
    itoa(header->mode, str, 16);
    terminal_putstring(str);
    terminal_putstring(" stratum=");
    itoa(header->stratum, str, 10);
    terminal_putstring(str);
    terminal_putstring(" poll=");
    itoa(header->poll, str, 10);
    terminal_putstring(str);
    terminal_putstring(" precision=");
    itoa(header->precision, str, 10);
    terminal_putstring(str);
    terminal_putstring(" rootDelay=");
    itoa(header->root_delay, str, 16);
    terminal_putstring(str);
    terminal_putstring(" rootDispersion=");
    itoa(header->root_dispersion, str, 16);
    terminal_putstring(str);
    terminal_putstring(" refId=");
    itoa(header->ref_id, str, 16);
    terminal_putstring(str);
    terminal_putstring(" refTimestamp=");
    itoa(header->ref_timestamp, str, 16);
    terminal_putstring(str);
    terminal_putstring(" origTimestamp=");
    itoa(header->orig_timestamp, str, 16);
    terminal_putstring(str);
    terminal_putstring(" recvTimestamp=");
    itoa(header->recv_timestamp, str, 16);
    terminal_putstring(str);
    terminal_putstring(" sendTimestamp=");
    itoa(header->send_timestamp, str, 16);
    terminal_putstring(str);
    terminal_putstring(" unixEpoch=");
    itoa(time, str, 10);
    terminal_putstring(str);
    terminal_putchar('\n');
}