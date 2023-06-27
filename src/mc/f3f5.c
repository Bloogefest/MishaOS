#include "f3f5.h"

#include "mcprotocol.h"
#include "../net/dns.h"
#include "../terminal.h"
#include "../string.h"
#include "../stdlib.h"
#include "../pit.h"

static const char* HOST = "game.f3f5.online";
static const uint16_t PORT = 25607;

static ipv4_addr_t addr;

void f3f5_on_data(tcp_conn_t* conn, const uint8_t* data, uint32_t len);
void f3f5_on_login_connect(tcp_conn_t* conn);
void f3f5_on_error(tcp_conn_t* conn, uint32_t error);
void f3f5_on_state(tcp_conn_t* conn, uint32_t old, uint32_t new);

void f3f5_handle_status_response(tcp_conn_t* conn, void* data) {
    mc_status_response_t* packet = (mc_status_response_t*) data;
    terminal_putchar('\n');
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
                terminal_putchar('\n');
                continue;
            }
        }

        terminal_putchar(*description++);
    }

    terminal_putstring("\n\n");
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
    terminal_putstring("Compression enabled.\n");
    mc_set_compression(packet->threshold);
}

void f3f5_on_data(tcp_conn_t* conn, const uint8_t* data, uint32_t len) {
    mc_read_packet(conn, data, len);
}

void f3f5_on_error(tcp_conn_t* conn, uint32_t error) {
    char num[20];
    terminal_putstring("Error occurred in the connection: ");
    itoa(error, num, 10);
    terminal_putstring(num);
    terminal_putstring("\n");
}

void f3f5_on_ping_connect(tcp_conn_t* conn) {
    terminal_putstring("Connected to the pomoyka F3F5 (ping).\n");
    mc_packet_decoders = mc_status_decoders;
    mc_packet_handlers = mc_status_handlers;
    mc_send_handshake(conn, 762, HOST, PORT, 1);
    mc_send_status_request(conn);
}

void f3f5_on_login_connect(tcp_conn_t* conn) {
    terminal_putstring("Connected to the pomoyka F3F5 (login).\n");
    mc_packet_decoders = mc_login_decoders;
    mc_packet_handlers = mc_login_handlers;
    mc_send_handshake(conn, 762, HOST, PORT, 2);
    char name[] = {'M', 'i', 's', 'h', 'a', 'O', 'S', '_', '0', '0', '0', 0};
    itoa(pit_get_ticks() % 1000, name + 8, 10);
    mc_send_login_start(conn, name);
    terminal_putstring("Logging in as ");
    terminal_putstring(name);
    terminal_putchar('\n');
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

    terminal_putstring("[Pomoyka F3F5] TCP state changed: ");
    terminal_putstring(TCP_STATES[old]);
    terminal_putstring(" -> ");
    terminal_putstring(TCP_STATES[new]);
    terminal_putstring("\n");
}

void f3f5_tcp_connect(void* ctx, const char* host, const net_buf_t* buf) {
    if (!dns_get_ip4_a(&addr, buf)) {
        terminal_putstring("Unable to find IP for host ");
        terminal_putstring(host);
        terminal_putchar('\n');
        return;
    } else {
        char str[16];
        terminal_putstring("Found IP for host ");
        terminal_putstring(host);
        terminal_putstring(": ");
        ip4toa(&addr, str);
        terminal_putstring(str);
        terminal_putchar('\n');
    }

    tcp_conn_t* conn = tcp_new_conn();
    conn->on_data = f3f5_on_data;
    conn->on_connect = f3f5_on_ping_connect;
    conn->on_error = f3f5_on_error;
    conn->on_state = f3f5_on_state;
    tcp_connect(conn, &addr, PORT);
}

void f3f5_connect() {
    mc_init();

    mc_status_handlers[0x00] = f3f5_handle_status_response;
    mc_status_handlers[0x01] = f3f5_handle_ping_response;

    mc_login_handlers[0x03] = f3f5_handle_set_compression;

    terminal_putstring("Connecting to the pomoyka F3F5...\n");
    dns_query_host(HOST, 1, 0, f3f5_tcp_connect);
}