#pragma once

#include "ipv4.h"

#define TCP_WINDOW_SIZE 8192
#define TCP_MSL 120000

#define SEQ_CMP(x, y, op) ((int) ((x) - (y)) op 0)

#define TCP_FIN (1 << 0)
#define TCP_SYN (1 << 1)
#define TCP_RST (1 << 2)
#define TCP_PSH (1 << 3)
#define TCP_ACK (1 << 4)
#define TCP_URG (1 << 5)

#define TCP_OPT_END 0
#define TCP_OPT_NOP 1
#define TCP_OPT_MSS 2

#define TCP_CLOSED 0
#define TCP_LISTEN 1
#define TCP_SYN_SENT 2
#define TCP_SYN_RECEIVED 3
#define TCP_ESTABLISHED 4
#define TCP_FIN_WAIT_1 5
#define TCP_FIN_WAIT_2 6
#define TCP_CLOSE_WAIT 7
#define TCP_CLOSING 8
#define TCP_LAST_ACK 9
#define TCP_TIME_WAIT 10

#define TCP_CONN_RESET 1
#define TCP_CONN_REFUSED 2
#define TCP_CONN_CLOSING 3

typedef struct tcp_header_s {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t off;
    uint8_t flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed)) tcp_header_t;

typedef struct tcp_options_s {
    uint16_t mss;
} tcp_options_t;

typedef struct tcp_snd_state_s {
    uint32_t una;
    uint32_t nxt;
    uint32_t wnd;
    uint32_t urp;
    uint32_t wl1;
    uint32_t wl2;
    uint32_t iss;
} tcp_snd_state_t;

typedef struct tcp_rcv_state_s {
    uint32_t nxt;
    uint32_t wnd;
    uint32_t urp;
    uint32_t irs;
} tcp_rcv_state_t;

typedef struct tcp_conn_s {
    struct tcp_conn_s* prev;
    struct tcp_conn_s* next;
    uint32_t state;
    net_intf_t* intf;
    ipv4_addr_t local_addr;
    ipv4_addr_t next_addr;
    ipv4_addr_t remote_addr;
    uint16_t local_port;
    uint16_t remote_port;
    tcp_snd_state_t snd;
    tcp_rcv_state_t rcv;
    net_buf_t* resequence;
    uint32_t msl_wait;
    void* ctx;
    void(*on_connect)(struct tcp_conn_s* conn);
    void(*on_error)(struct tcp_conn_s* conn, uint32_t error);
    void(*on_state)(struct tcp_conn_s* conn, uint32_t old_state, uint32_t new_state);
    void(*on_data)(struct tcp_conn_s* conn, const uint8_t* data, uint32_t len);
} tcp_conn_t;

extern tcp_conn_t* tcp_conn_list;

void tcp_init();
void tcp_recv(net_intf_t* intf, const ipv4_header_t* header, net_buf_t* packet);
void tcp_poll();
void tcp_swap(tcp_header_t* header);

tcp_conn_t* tcp_new_conn();
uint8_t tcp_connect(tcp_conn_t* conn, const ipv4_addr_t* addr, uint16_t port);
void tcp_close(tcp_conn_t* conn);
void tcp_send(tcp_conn_t* conn, const void* data, uint32_t len);