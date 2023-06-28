#include "tcp.h"

#include "buf.h"
#include "checksum.h"
#include "ipv4.h"
#include "port.h"
#include "route.h"
#include "in.h"
#include "../pit.h"
#include "../rtc.h"
#include "../gpd.h"
#include "../stdlib.h"
#include "../string.h"
#include "../terminal.h"

static uint32_t base_isn;
static tcp_conn_t* free_conn_list;

tcp_conn_t* tcp_conn_list;

typedef struct checksum_header_s {
    ipv4_addr_t src;
    ipv4_addr_t dst;
    uint8_t reserved;
    uint8_t protocol;
    uint16_t len;
} __attribute__((packed)) checksum_header_t;

static uint8_t tcp_parse_options(tcp_options_t* options, const uint8_t* ptr, const uint8_t* end) {
    memset(options, 0, sizeof(*options));

    while (ptr < end) {
        uint8_t type = *ptr++;
        if (type == TCP_OPT_NOP) {
            continue;
        } else if (type == TCP_OPT_END) {
            break;
        }

        uint8_t len = *ptr++;
        if (len < 2) {
            return 0;
        }

        const uint8_t* next = ptr + len - 2;
        if (next > end) {
            return 0;
        }

        switch (type) {
            case TCP_OPT_MSS:
                options->mss = net_swap16(*(uint16_t*) ptr);
        }

        ptr = next;
    }

    return 1;
}

static void tcp_dump(const net_buf_t* packet) {
#ifndef NET_DEBUG
    return;
#endif

    if (packet->start + sizeof(tcp_header_t) > packet->end) {
        return;
    }

    char str[30];
    checksum_header_t* pheader = (checksum_header_t*) (packet->start - sizeof(checksum_header_t));
    const tcp_header_t* header = (const tcp_header_t*) packet->start;
    uint16_t src_port = net_swap16(header->src_port);
    uint16_t dst_port = net_swap16(header->dst_port);
    uint32_t seq = net_swap32(header->seq);
    uint32_t ack = net_swap32(header->ack);
    uint16_t window_size = net_swap16(header->window_size);
    uint16_t checksum = net_swap16(header->checksum);
    uint16_t urgent = net_swap16(header->urgent);

    uint16_t actual_checksum = net_checksum(packet->start - sizeof(checksum_header_t), packet->end);
    uint32_t header_len = header->off >> 2;
    uint32_t data_len = (packet->end - packet->start) - header_len;

    terminal_putstring("   TCP: src=");
    ip4ptoa(&pheader->src, src_port, str);
    terminal_putstring(str);
    terminal_putstring(" dst=");
    ip4ptoa(&pheader->dst, dst_port, str);
    terminal_putstring(str);
    terminal_putstring(" seq=");
    itoa(seq, str, 10);
    terminal_putstring(str);
    terminal_putstring(" ack=");
    itoa(ack, str, 10);
    terminal_putstring(str);
    terminal_putstring("\n   TCP: dataLen=");
    itoa(data_len, str, 10);
    terminal_putstring(str);
    terminal_putstring(" flags=0x");
    itoa(header->flags, str, 16);
    terminal_putstring(str);
    terminal_putstring(" window=");
    itoa(window_size, str, 10);
    terminal_putstring(str);
    terminal_putstring(" urgent=");
    itoa(urgent, str, 10);
    terminal_putstring(str);
    terminal_putstring(" checksum=");
    itoa(checksum, str, 10);
    terminal_putstring(str);
    terminal_putchar(actual_checksum ? '!' : ' ');
    terminal_putchar('\n');

    if (header_len > sizeof(tcp_header_t)) {
        const uint8_t* ptr = packet->start + sizeof(tcp_header_t);
        const uint8_t* end = ptr + header_len;

        tcp_options_t options;
        tcp_parse_options(&options, ptr, end);

        if (options.mss) {
            terminal_putstring("   TCP: mss=");
            itoa(options.mss, str, 10);
            terminal_putstring(str);
            terminal_putchar('\n');
        }
    }
}

static void tcp_set_state(tcp_conn_t* conn, uint32_t state) {
    uint32_t old_state = conn->state;
    conn->state = state;
    if (conn->on_state) {
        conn->on_state(conn, old_state, state);
    }
}

static tcp_conn_t* tcp_alloc() {
    tcp_conn_t* conn;
    if (free_conn_list) {
        conn = free_conn_list;
        free_conn_list = free_conn_list->next;
    } else {
        conn = pfa_request_page(&pfa);
    }

    memset(conn, 0, sizeof(tcp_conn_t));
    return conn;
}

static void tcp_free(tcp_conn_t* conn) {
    if (conn->state != TCP_CLOSED) {
        tcp_set_state(conn, TCP_CLOSED);

        for (net_buf_t* packet = conn->resequence; packet;) {
            net_buf_t* next = packet->link;
            net_free_buf(packet);
            packet = next;
        }
    }

    if (free_conn_list) {
        free_conn_list->prev = conn;
    }

    conn->next = free_conn_list;
    conn->prev = 0;
    free_conn_list = conn;
}

static void tcp_send_packet(tcp_conn_t* conn, uint32_t seq, uint8_t flags, const void* data, uint32_t len) {
    net_buf_t* packet = net_alloc_buf();
    tcp_header_t* header = (tcp_header_t*) packet->start;
    header->src_port = conn->local_port;
    header->dst_port = conn->remote_port;
    header->seq = seq;
    header->ack = flags & TCP_ACK ? conn->rcv.nxt : 0;
    header->off = 0;
    header->flags = flags;
    header->window_size = TCP_WINDOW_SIZE;
    header->checksum = 0;
    header->urgent = 0;
    tcp_swap(header);

    uint8_t* ptr = packet->start + sizeof(tcp_header_t);
    if (flags & TCP_SYN) {
        ptr[0] = TCP_OPT_MSS;
        ptr[1] = 4;
        *(uint16_t*)(ptr + 2) = net_swap16(1460);
        ptr += ptr[1];
    }

    while ((ptr - packet->start) & 3) {
        *ptr++ = 0;
    }

    header->off = (ptr - packet->start) << 2;

    memcpy(ptr, data, len);
    packet->end = ptr + len;

    checksum_header_t* pheader = (checksum_header_t*) (packet->start - sizeof(checksum_header_t));
    pheader->src = conn->local_addr;
    pheader->dst = conn->remote_addr;
    pheader->reserved = 0;
    pheader->protocol = IP_PROTOCOL_TCP;
    pheader->len = net_swap16(packet->end - packet->start);

    uint16_t checksum = net_checksum(packet->start - sizeof(checksum_header_t), packet->end);
    header->checksum = net_swap16(checksum);

    tcp_dump(packet);
    ipv4_intf_send(conn->intf, &conn->next_addr, &conn->remote_addr, IP_PROTOCOL_TCP, packet);

    conn->snd.nxt += len;
    if (flags & (TCP_SYN | TCP_FIN)) {
        ++conn->snd.nxt;
    }
}

static tcp_conn_t* tcp_find(const ipv4_addr_t* src_addr, uint16_t src_port, const ipv4_addr_t* dst_addr, uint16_t dst_port) {
    for (tcp_conn_t* conn = tcp_conn_list; conn; conn = conn->next) {
        if (src_addr->bits == conn->remote_addr.bits && dst_addr->bits == conn->local_addr.bits
            && src_port == conn->remote_port && dst_port == conn->local_port) {
            return conn;
        }
    }

    return 0;
}

static void tcp_error(tcp_conn_t* conn, uint32_t error) {
    if (conn->on_error) {
        conn->on_error(conn, error);
    }

    tcp_free(conn);
}

void tcp_init() {
    date_time_t date;
    rtc_get_time(&date);
    abs_time time = join_time(&date);

    base_isn = (time * 1000 - pit_get_ticks()) * 250;
}

static void tcp_recv_closed(checksum_header_t* pheader, tcp_header_t* header) {
    if (header->flags & TCP_RST) {
        return;
    }

    const ipv4_addr_t* dst_addr = &pheader->src;
    const net_route_t* route = net_find_route(dst_addr);
    if (!route) {
        return;
    }

    tcp_conn_t rst_conn;
    memset(&rst_conn, 0, sizeof(tcp_conn_t));

    rst_conn.intf = route->intf;
    rst_conn.local_addr = pheader->dst;
    rst_conn.local_port = header->dst_port;
    rst_conn.remote_addr = pheader->src;
    rst_conn.remote_port = header->src_port;
    rst_conn.next_addr = *net_next_addr(route, dst_addr);

    if (header->flags & TCP_ACK) {
        tcp_send_packet(&rst_conn, header->ack, TCP_RST, 0, 0);
    } else {
        uint32_t header_len = header->off >> 2;
        uint32_t data_len = pheader->len - header_len;

        rst_conn.rcv.nxt = header->seq + data_len;

        tcp_send_packet(&rst_conn, 0, TCP_RST | TCP_ACK, 0, 0);
    }
}

static void tcp_recv_syn_sent(tcp_conn_t* conn, tcp_header_t* header) {
    uint32_t flags = header->flags;
    if (flags & TCP_ACK) {
        if (SEQ_CMP(header->ack, conn->snd.iss, <=) || SEQ_CMP(header->ack, conn->snd.nxt, >)) {
            if (~flags & TCP_RST) {
                tcp_send_packet(conn, header->ack, TCP_RST, 0, 0);
            }

            return;
        }
    }

    if (flags & TCP_RST) {
        if (flags & TCP_ACK) {
            tcp_error(conn, TCP_CONN_RESET);
        }

        return;
    }

    if (flags & TCP_SYN) {
        conn->rcv.irs = header->seq;
        conn->rcv.nxt = header->seq + 1;

        if (flags & TCP_ACK) {
            conn->snd.una = header->ack;
            conn->snd.wnd = header->window_size;
            conn->snd.wl1 = header->seq;
            conn->snd.wl2 = header->ack;

            tcp_set_state(conn, TCP_ESTABLISHED);
            tcp_send_packet(conn, conn->snd.nxt, TCP_ACK, 0, 0);

            if (conn->on_connect) {
                conn->on_connect(conn);
            }
        } else {
            tcp_set_state(conn, TCP_SYN_RECEIVED);

            --conn->snd.nxt;
            tcp_send_packet(conn, conn->snd.nxt, TCP_SYN | TCP_ACK, 0, 0);
        }
    }
}

static void tcp_recv_rst(tcp_conn_t* conn, tcp_header_t* header) {
    switch (conn->state) {
        case TCP_SYN_RECEIVED:
            tcp_error(conn, TCP_CONN_REFUSED);
            break;

        case TCP_ESTABLISHED:
        case TCP_FIN_WAIT_1:
        case TCP_FIN_WAIT_2:
        case TCP_CLOSE_WAIT:
            tcp_error(conn, TCP_CONN_RESET);
            break;

        case TCP_CLOSING:
        case TCP_LAST_ACK:
        case TCP_TIME_WAIT:
            tcp_free(conn);
            break;
    }
}

static void tcp_recv_syn(tcp_conn_t* conn, tcp_header_t* header) {
    tcp_send_packet(conn, 0, TCP_RST | TCP_ACK, 0, 0);
    tcp_error(conn, TCP_CONN_RESET);
}

static void tcp_recv_ack(tcp_conn_t* conn, tcp_header_t* header) {
    switch (conn->state) {
        case TCP_SYN_RECEIVED:
            if (conn->snd.una <= header->ack && header->ack <= conn->snd.nxt) {
                conn->snd.wnd = header->window_size;
                conn->snd.wl1 = header->seq;
                conn->snd.wl2 = header->ack;
                tcp_set_state(conn, TCP_ESTABLISHED);
            } else {
                tcp_send_packet(conn, header->ack, TCP_RST, 0, 0);
            }

            break;

        case TCP_ESTABLISHED:
        case TCP_FIN_WAIT_1:
        case TCP_FIN_WAIT_2:
        case TCP_CLOSE_WAIT:
        case TCP_CLOSING:
            if (SEQ_CMP(conn->snd.una, header->ack, <=) && SEQ_CMP(header->ack, conn->snd.nxt, <=)) {
                conn->snd.una = header->ack;

                if (SEQ_CMP(conn->snd.wl1, header->seq, <) || (conn->snd.wl1 == header->seq && SEQ_CMP(conn->snd.wl2, header->ack, <=))) {
                    conn->snd.wnd = header->window_size;
                    conn->snd.wl1 = header->seq;
                    conn->snd.wl2 = header->ack;
                }
            }

            if (SEQ_CMP(header->ack, conn->snd.una, <=)) {
                // TODO: Do anything?
            }

            if (SEQ_CMP(header->ack, conn->snd.nxt, >)) {
                tcp_send_packet(conn, conn->snd.nxt, TCP_ACK, 0, 0);
                return;
            }

            if (SEQ_CMP(header->ack, conn->snd.nxt, >=)) {
                if (conn->state == TCP_FIN_WAIT_1) {
                    tcp_set_state(conn, TCP_FIN_WAIT_2);
                } else if (conn->state == TCP_CLOSING) {
                    tcp_set_state(conn, TCP_TIME_WAIT);
                    conn->msl_wait = pit_get_ticks() + 2 * TCP_MSL;
                }
            }

            break;

        case TCP_LAST_ACK:
            if (SEQ_CMP(header->ack, conn->snd.nxt, >=)) {
                tcp_free(conn);
            }

            break;

        case TCP_TIME_WAIT:
            break;
    }
}

static void tcp_recv_insert(tcp_conn_t* conn, net_buf_t* packet) {
    net_buf_t* prev = 0;
    net_buf_t* cur;

    uint32_t data_len = packet->end - packet->start;
    uint32_t packet_end = packet->seq + data_len;

    for (cur = conn->resequence; cur; cur = cur->link) {
        if (SEQ_CMP(packet->seq, cur->seq, <=)) {
            break;
        }

        prev = cur;
    }

    if (prev) {
        uint32_t prev_end = prev->seq + prev->end - prev->start;

        if (SEQ_CMP(prev_end, packet_end, >=)) {
            net_free_buf(packet);
            return;
        } else if (SEQ_CMP(prev_end, packet->seq, >)) {
            prev->end -= prev_end - packet->seq;
        }
    }

    if (packet->flags & TCP_FIN) {
        while (cur) {
            net_buf_t* next = cur->link;
            cur->link = 0;
            net_free_buf(cur);
            cur = next;
        }
    }

    while (cur) {
        uint32_t packet_end = packet->seq + data_len;
        uint32_t cur_end = cur->seq + cur->end - cur->start;

        if (SEQ_CMP(packet_end, cur->seq, <)) {
            break;
        }

        if (SEQ_CMP(packet_end, cur_end, <)) {
            packet->end -= packet_end - cur->seq;
            break;
        }

        net_buf_t* next = cur->link;
        cur->link = 0;
        net_free_buf(cur);
        cur = next;
    }

    if (cur) {
        cur->link = packet->link;
        packet->link = cur;
    }

    conn->resequence = packet;
}

static void tcp_recv_process(tcp_conn_t* conn) {
    for (net_buf_t* packet = conn->resequence; packet;) {
        net_buf_t* next = packet->link;
        if (conn->rcv.nxt != packet->seq) {
            break;
        }

        uint32_t data_len = packet->end - packet->start;
        conn->rcv.nxt += data_len;

        if (conn->on_data) {
            conn->on_data(conn, packet->start, data_len);
        }

        net_free_buf(packet);
        packet = next;
        conn->resequence = packet;
    }
}

static void tcp_recv_data(tcp_conn_t* conn, net_buf_t* packet) {
    switch (conn->state) {
        case TCP_SYN_RECEIVED:
            break;

        case TCP_ESTABLISHED:
        case TCP_FIN_WAIT_1:
        case TCP_FIN_WAIT_2:
            ++packet->ref_count;

            tcp_recv_insert(conn, packet);
            tcp_recv_process(conn);
            tcp_send_packet(conn, conn->snd.nxt, TCP_ACK, 0, 0);
            break;

        default:
            break;
    }
}

static void tcp_recv_fin(tcp_conn_t* conn, tcp_header_t* header) {
    conn->rcv.nxt = header->seq + 1;
    tcp_send_packet(conn, conn->snd.nxt, TCP_ACK, 0, 0);

    switch (conn->state) {
        case TCP_SYN_RECEIVED:
        case TCP_ESTABLISHED:
            tcp_set_state(conn, TCP_CLOSE_WAIT);
            break;

        case TCP_FIN_WAIT_1:
            if (SEQ_CMP(header->ack, conn->snd.nxt, >=)) {
                tcp_set_state(conn, TCP_TIME_WAIT);
                conn->msl_wait = pit_get_ticks() + 2 * TCP_MSL;
            } else {
                tcp_set_state(conn, TCP_CLOSING);
            }

            break;

        case TCP_FIN_WAIT_2:
            tcp_set_state(conn, TCP_TIME_WAIT);
            conn->msl_wait = pit_get_ticks() + 2 * TCP_MSL;
            break;

        case TCP_CLOSE_WAIT:
        case TCP_CLOSING:
        case TCP_LAST_ACK:
            break;

        case TCP_TIME_WAIT:
            conn->msl_wait = pit_get_ticks() + 2 * TCP_MSL;
            break;
    }
}

static void tcp_recv_general(tcp_conn_t* conn, tcp_header_t* header, net_buf_t* packet) {
    uint32_t flags = header->flags;
    uint32_t data_len = packet->end - packet->start;

    if (!(SEQ_CMP(conn->rcv.nxt, header->seq, <=) && SEQ_CMP(header->seq + data_len, conn->rcv.nxt + conn->rcv.wnd, <=))) {
        if (~flags & TCP_RST) {
            tcp_send_packet(conn, conn->snd.nxt, TCP_ACK, 0, 0);
        }

        return;
    }

    if (flags & TCP_RST) {
        tcp_recv_rst(conn, header);
        return;
    }

    if (flags & TCP_SYN) {
        tcp_recv_syn(conn, header);
    }

    if (~flags & TCP_ACK) {
        return;
    }

    tcp_recv_ack(conn, header);

    if (data_len) {
        tcp_recv_data(conn, packet);
    }

    if (flags & TCP_FIN) {
        tcp_recv_fin(conn, header);
    }
}

void tcp_recv(net_intf_t* intf, const ipv4_header_t* ip_header, net_buf_t* packet) {
    if (packet->start + sizeof(tcp_header_t) > packet->end) {
        return;
    }

    ipv4_addr_t src_addr = ip_header->src;
    ipv4_addr_t dst_addr = ip_header->dst;
    uint8_t protocol = ip_header->protocol;

    checksum_header_t* pheader = (checksum_header_t*) (packet->start - sizeof(checksum_header_t));
    pheader->src = src_addr;
    pheader->dst = dst_addr;
    pheader->reserved = 0;
    pheader->protocol = protocol;
    pheader->len = net_swap16(packet->end - packet->start);

    tcp_dump(packet);

    if (net_checksum(packet->start - sizeof(checksum_header_t), packet->end)) {
        return;
    }

    tcp_header_t* header = (tcp_header_t*) packet->start;
    tcp_swap(header);
    pheader->len = net_swap16(pheader->len);

    tcp_conn_t* conn = tcp_find(&pheader->src, header->src_port, &pheader->dst, header->dst_port);
    if (!conn || conn->state == TCP_CLOSED) {
        tcp_recv_closed(pheader, header);
        return;
    }

    if (conn->state == TCP_SYN_SENT) {
        tcp_recv_syn_sent(conn, header);
    } else if (conn->state != TCP_LISTEN) {
        uint32_t header_len = header->off >> 2;
        packet->start += header_len;
        packet->seq = header->seq;
        packet->flags = header->flags;
        tcp_recv_general(conn, header, packet);
    }
}

void tcp_poll() {
    for (tcp_conn_t* conn = tcp_conn_list; conn;) {
        tcp_conn_t* next = conn->next;
        if (conn->state == TCP_TIME_WAIT && SEQ_CMP(pit_get_ticks(), conn->msl_wait, >=)) {
            tcp_free(conn);
        }

        conn = next;
    }
}

void tcp_swap(tcp_header_t* header) {
    header->src_port = net_swap16(header->src_port);
    header->dst_port = net_swap16(header->dst_port);
    header->seq = net_swap32(header->seq);
    header->ack = net_swap32(header->ack);
    header->window_size = net_swap16(header->window_size);
    header->checksum = net_swap16(header->checksum);
    header->urgent = net_swap16(header->urgent);
}

tcp_conn_t* tcp_new_conn() {
    return tcp_alloc();
}

uint8_t tcp_connect(tcp_conn_t* conn, const ipv4_addr_t* addr, uint16_t port) {
    const net_route_t* route = net_find_route(addr);
    if (!route) {
        return 0;
    }

    net_intf_t* intf = route->intf;
    conn->intf = intf;
    conn->local_addr = intf->ip_addr;
    conn->next_addr = *net_next_addr(route, addr);
    conn->remote_addr = *addr;
    conn->local_port = net_ephemeral_port();
    conn->remote_port = port;

    uint32_t isn = base_isn + pit_get_ticks() * 250;

    conn->snd.una = isn;
    conn->snd.nxt = isn;
    conn->snd.wnd = TCP_WINDOW_SIZE;
    conn->snd.urp = 0;
    conn->snd.wl1 = 0;
    conn->snd.wl2 = 0;
    conn->snd.iss = isn;

    conn->rcv.nxt = 0;
    conn->rcv.wnd = TCP_WINDOW_SIZE;
    conn->rcv.urp = 0;
    conn->rcv.irs = 0;

    if (tcp_conn_list) {
        tcp_conn_list->prev = conn;
    }

    conn->next = tcp_conn_list;
    tcp_conn_list = conn;

    tcp_send_packet(conn, conn->snd.nxt, TCP_SYN, 0, 0);
    tcp_set_state(conn, TCP_SYN_SENT);

    return 1;
}

void tcp_close(tcp_conn_t* conn) {
    switch (conn->state) {
        case TCP_CLOSED:
        case TCP_LISTEN:
        case TCP_SYN_SENT:
            tcp_free(conn);
            break;

        case TCP_SYN_RECEIVED:
            tcp_send_packet(conn, conn->snd.nxt, TCP_FIN | TCP_ACK, 0, 0);
            tcp_set_state(conn, TCP_FIN_WAIT_1);
            break;

        case TCP_ESTABLISHED:
            tcp_send_packet(conn, conn->snd.nxt, TCP_FIN | TCP_ACK, 0, 0);
            tcp_set_state(conn, TCP_FIN_WAIT_1);
            break;

        case TCP_FIN_WAIT_1:
        case TCP_FIN_WAIT_2:
        case TCP_CLOSING:
        case TCP_LAST_ACK:
        case TCP_TIME_WAIT:
            if (conn->on_error) {
                conn->on_error(conn, TCP_CONN_CLOSING);
            }

            break;

        case TCP_CLOSE_WAIT:
            tcp_send_packet(conn, conn->snd.nxt, TCP_FIN | TCP_ACK, 0, 0);
            tcp_set_state(conn, TCP_LAST_ACK);
            break;
    }
}

void tcp_send(tcp_conn_t* conn, const void* data, uint32_t len) {
    tcp_send_packet(conn, conn->snd.nxt, TCP_ACK, data, len);
}