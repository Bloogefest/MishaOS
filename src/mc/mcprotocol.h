#pragma once

#include <net/tcp.h>
#include <stdint.h>

// --------------------- CLIENTBOUND STATUS --------------------- //

typedef struct mc_status_response_s {
    char* json;
} mc_status_response_t;

typedef struct mc_ping_response_t {
    uint64_t id;
} mc_ping_response_t;

// --------------------- CLIENTBOUND LOGIN --------------------- //

typedef struct mc_set_compression_s {
    uint32_t threshold;
} mc_set_compression_t;

// ------------------------------------------------------------- //

typedef struct mc_packet_decoder_s {
    void*(*decoder)(net_buf_t* buf);
    void(*deallocator)(void* data);
} mc_packet_decoder_t;

typedef void(*mc_packet_handler_t)(tcp_conn_t* conn, void* data);

extern mc_packet_handler_t mc_status_handlers[0x02];
extern mc_packet_handler_t mc_login_handlers[0x05];
extern mc_packet_handler_t mc_play_handlers[0xFF];
extern mc_packet_decoder_t mc_status_decoders[0x02];
extern mc_packet_decoder_t mc_login_decoders[0x05];
extern mc_packet_decoder_t mc_play_decoders[0xFF];
extern mc_packet_decoder_t* mc_packet_decoders;
extern mc_packet_handler_t* mc_packet_handlers;

void mc_init();
uint8_t mc_varlen(uint32_t value);
uint8_t* mc_varencode(uint8_t* ptr, uint32_t value);
uint8_t* mc_strencode(uint8_t* ptr, const char* str);
uint32_t mc_vardecode(const uint8_t* ptr, uint8_t** dst);
char* mc_strdecode(const uint8_t* ptr, uint8_t** dst);

void mc_read_packet(tcp_conn_t* conn, const uint8_t* buf, uint32_t len);
void mc_send_packet(tcp_conn_t* conn);

void mc_write_var(uint32_t value);
void mc_write_str(const char* str);
void mc_write8(uint8_t value);
void mc_write16(uint16_t value);
void mc_write32(uint32_t value);
void mc_write64(uint64_t value);

uint32_t mc_read_var(net_buf_t* buf);
char* mc_read_str(net_buf_t* buf);
uint8_t mc_read8(net_buf_t* buf);
uint16_t mc_read16(net_buf_t* buf);
uint32_t mc_read32(net_buf_t* buf);
uint64_t mc_read64(net_buf_t* buf);

void mc_set_compression(uint32_t threshold);

void mc_send_handshake(tcp_conn_t* conn, uint32_t protocol, const char* host, uint16_t port, uint8_t state);

void mc_send_status_request(tcp_conn_t* conn);
void mc_send_ping_request(tcp_conn_t* conn, uint64_t id);

void mc_send_login_start(tcp_conn_t* conn, const char* name);