#include "f3f5.h"

#include <mc/mcprotocol.h>
#include <net/dns.h>
#include <lib/kprintf.h>
#include <lib/string.h>
#include <lib/stdlib.h>
#include <sys/pit.h>

static const char* HOST = "game.f3f5.xyz";
static const uint16_t PORT = 55232;

static ipv4_addr_t addr;

void f3f5_on_data(tcp_conn_t* conn, const uint8_t* data, uint32_t len);
void f3f5_on_login_connect(tcp_conn_t* conn);
void f3f5_on_error(tcp_conn_t* conn, uint32_t error);
void f3f5_on_state(tcp_conn_t* conn, uint32_t old, uint32_t new);

void f3f5_handle_status_response(tcp_conn_t* conn, void* data) {
    mc_status_response_t* packet = (mc_status_response_t*) data;
    putchar('\n');
    char* description = strstr(packet->json, "\"text\"") + 6;
    while (*description++ != '"');
    while (*description != '"') {
        if (((uint8_t) *description) == 0xC2) {
            description += 3;
            continue;
        }

        if (*description == '\\') {
            ++description;
            if (*description == 'n') {
                ++description;
                putchar('\n');
                continue;
            }
        }

        putchar(*description++);
    }

    puts("\n");
    mc_send_ping_request(conn, pit_get_ticks());
}

void f3f5_handle_ping_response(tcp_conn_t* conn, void* data) {
    tcp_close(conn);
    conn = tcp_new_conn();
    conn->on_data = f3f5_on_data;
    conn->on_connect = f3f5_on_login_connect;
    conn->on_error = f3f5_on_error;
    conn->on_state = f3f5_on_state;
    tcp_connect(conn, &addr, PORT);
}

void f3f5_handle_set_compression(tcp_conn_t* conn, void* data) {
    mc_set_compression_t* packet = (mc_set_compression_t*) data;
    puts("Compression enabled.");
    mc_set_compression(packet->threshold);
}

void f3f5_on_data(tcp_conn_t* conn, const uint8_t* data, uint32_t len) {
    mc_read_packet(conn, data, len);
}

void f3f5_on_error(tcp_conn_t* conn, uint32_t error) {
    kprintf("Error occurred in the connection: %ld\n", error);
}

void f3f5_on_ping_connect(tcp_conn_t* conn) {
    puts("Connected to the pomoyka F3F5 (ping).");
    mc_packet_decoders = mc_status_decoders;
    mc_packet_handlers = mc_status_handlers;
    mc_send_handshake(conn, 762, HOST, PORT, 1);
    mc_send_status_request(conn);
}

void f3f5_on_login_connect(tcp_conn_t* conn) {
    puts("Connected to the pomoyka F3F5 (login).");
    mc_packet_decoders = mc_login_decoders;
    mc_packet_handlers = mc_login_handlers;
    mc_send_handshake(conn, 762, HOST, PORT, 2);
    char name[] = {'M', 'i', 's', 'h', 'a', 'O', 'S', '_', '0', '0', '0', 0};
    itoa(pit_get_ticks() % 1000, name + 8, 10);
    mc_send_login_start(conn, name);
    kprintf("Logging in as %s\n", name);
}

void f3f5_on_state(tcp_conn_t* conn, uint32_t old, uint32_t new) {
    static const char* TCP_STATES[] ={
            "CLOSED",
            "LISTEN",
            "SYN-SENT",
            "SYN-RECEIVED",
            "ESTABLISHED",
            "FIN-WAIT-1",
            "FIN-WAIT-2",
            "CLOSE-WAIT",
            "CLOSING",
            "LAST-ACK",
            "TIME-WAIT"
    };

    kprintf("[Pomoyka F3F5] TCP state changed: %s -> %s\n", TCP_STATES[old], TCP_STATES[new]);
}

void f3f5_tcp_connect(void* ctx, const char* host, const net_buf_t* buf) {
    if (!dns_get_ip4_a(&addr, buf)) {
        kprintf("Unable to find IP for host %s\n", host);
        return;
    } else {
        char str[16];
        ip4toa(&addr, str);
        kprintf("Found IP for host %s: %s\n", host, str);
    }

    tcp_conn_t* conn = tcp_new_conn();
    conn->on_data = f3f5_on_data;
    conn->on_connect = f3f5_on_ping_connect;
    conn->on_error = f3f5_on_error;
    conn->on_state = f3f5_on_state;
    tcp_connect(conn, &addr, PORT);
}

void f3f5_connect(net_intf_t* intf) {
    mc_init();

    mc_status_handlers[0x00] = f3f5_handle_status_response;
    mc_status_handlers[0x01] = f3f5_handle_ping_response;

    mc_login_handlers[0x03] = f3f5_handle_set_compression;

    puts("Connecting to the pomoyka F3F5...");
    dns_query_host(HOST, 1, 0, f3f5_tcp_connect);
}