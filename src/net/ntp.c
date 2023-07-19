#include "ntp.h"

#include <net/buf.h>
#include <net/net.h>
#include <net/port.h>
#include <net/in.h>
#include <net/udp.h>
#include <lib/kprintf.h>
#include <lib/stdlib.h>
#include <sys/rtc.h>

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
    kprintf("Setting time to %s\n", str);

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

    kprintf("   NTP: mode=%02x stratum=%d poll=%d precision=%d rootDelay=%08lx rootDispersion=%08lx, refId=%08lx\n",
            header->mode, header->stratum, header->poll, header->precision, net_swap32(header->root_delay),
            net_swap32(header->root_dispersion), net_swap32(header->ref_id));
    kprintf("   NTP: refTimestamp=%016llx origTimestamp=%016llx recvTimestamp=%016llx\n",
            net_swap64(header->ref_timestamp), net_swap64(header->orig_timestamp),
            net_swap64(header->recv_timestamp));
    kprintf("   NTP: sendTimestamp=%016llx unixEpoch=%lu\n", net_swap64(header->send_timestamp), time);
}