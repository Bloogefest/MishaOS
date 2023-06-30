#include "mcprotocol.h"

#include "../heap.h"
#include "../stdlib.h"
#include "../string.h"
#include "../kprintf.h"

#define RX_BUF_SIZE 16384

mc_packet_handler_t mc_status_handlers[0x02];
mc_packet_handler_t mc_login_handlers[0x05];
mc_packet_handler_t mc_play_handlers[0xFF];

mc_packet_decoder_t mc_status_decoders[0x02];
mc_packet_decoder_t mc_login_decoders[0x05];
mc_packet_decoder_t mc_play_decoders[0xFF];

mc_packet_decoder_t* mc_packet_decoders;
mc_packet_handler_t* mc_packet_handlers;

static net_buf_t* tx_buf;
static uint8_t* rx_ptr;
static uint32_t rx_cnt = 0;
static uint32_t rx_len = 0;
static uint32_t rx_pos = 0;

static uint32_t compression;

void mc_dump(const net_buf_t* b) {
    uint8_t* ptr = b->start;

    kprintf("   MC: length=%ld", mc_vardecode(ptr, &ptr));

    if (compression) {
        uint32_t uncompressed = mc_vardecode(ptr, &ptr);
        kprintf(" uncompressed=");
        if (uncompressed) {
            kprintf("%ld", uncompressed);
            return;
        } else {
            kprintf("[not compressed]");
        }
    }

    kprintf(" id=0x%lx\n", mc_vardecode(ptr, &ptr));
}

uint8_t mc_varlen(uint32_t value) {
    uint32_t length = 1;

    while (value & ~0x7F) {
        ++length;
        value >>= 7;
    }

    return length;
}

uint8_t* mc_varencode(uint8_t* ptr, uint32_t value) {
    while (value & ~0x7F) {
        *ptr++ = (uint8_t) (value & 0x7F) | 0x80;
        value >>= 7;
    }

    *ptr++ = (uint8_t) value;
    return ptr;
}

uint8_t* mc_strencode(uint8_t* ptr, const char* str) {
    uint32_t len = strlen(str);
    ptr = mc_varencode(ptr, len);
    memcpy(ptr, str, len);
    return ptr + len;
}

uint32_t mc_vardecode(const uint8_t* buf, uint8_t** dst) {
    uint8_t* ptr = (uint8_t*) buf;
    uint32_t value = 0;
    uint8_t position = 0;
    while (1) {
        uint8_t byte = *ptr++;
        value |= (byte & 0x7F) << position;
        if ((byte & 0x80) == 0) {
            break;
        }

        position += 7;
        if (position >= 32) {
            puts("mc_vardecode: overflow");
            break;
        }
    }

    if (dst) {
        *dst = ptr;
    }

    return value;
}

char* mc_strdecode(const uint8_t* buf, uint8_t** dst) {
    uint8_t* ptr = (uint8_t*) buf;
    uint32_t len = mc_vardecode(ptr, &ptr);
    char* str = malloc(len + 1);
    memcpy(str, ptr, len);
    *dst = ptr + len;
    return str;
}

void mc_read_packet(tcp_conn_t* conn, const uint8_t* buf, uint32_t len) {
    if (rx_pos == 0) {
        rx_len = mc_vardecode(buf, 0);
        rx_len += mc_varlen(rx_len);
    }

    int32_t overflow = RX_BUF_SIZE - (rx_cnt + len);
    uint32_t rx_bytes = len;
    if (overflow < 0) {
        if ((uint32_t) (-overflow) >= len) {
            rx_bytes = 0;
        } else {
            rx_bytes += overflow;
        }
    }

    memcpy(rx_ptr + rx_cnt, buf, rx_bytes);
    rx_cnt += rx_bytes;
    rx_pos += len;

    if (rx_pos >= rx_len) {
        net_buf_t rx_buf;
        rx_buf.start = rx_ptr;
        rx_buf.end = rx_ptr + rx_cnt;
        mc_dump(&rx_buf);

        uint32_t packet_len = mc_vardecode(rx_buf.start, &rx_buf.start);
        packet_len += mc_varlen(packet_len);
        if (packet_len != rx_len) {
            puts("Invalid packet size.");
        } else {
            uint32_t id = mc_vardecode(rx_buf.start, &rx_buf.start);
            id %= 0xFF;

            mc_packet_handler_t handler = mc_packet_handlers[id];
            mc_packet_decoder_t* decoder = &mc_packet_decoders[id];

            if (decoder->decoder && handler) {
                void* packet = decoder->decoder(&rx_buf);
                handler(conn, packet);
                if (decoder->deallocator) {
                    decoder->deallocator(packet);
                }
            }
        }

        rx_cnt = 0;
        rx_pos = 0;
        rx_len = 0;
    }
}

void mc_wrap_send_packet(tcp_conn_t* conn, uint32_t id) {
    tx_buf->start -= mc_varlen(id);
    uint32_t len = tx_buf->end - tx_buf->start;
    tx_buf->start -= mc_varlen(len);
    mc_varencode(mc_varencode(tx_buf->start, len), id);
    mc_send_packet(conn);
}

void mc_send_packet(tcp_conn_t* conn) {
    mc_dump(tx_buf);
    tcp_send(conn, tx_buf->start, tx_buf->end - tx_buf->start);
    net_free_buf(tx_buf);
    tx_buf = net_alloc_buf();
}

void mc_write_var(uint32_t value) {
    tx_buf->end = mc_varencode(tx_buf->end, value);
}

void mc_write_str(const char* str) {
    tx_buf->end = mc_strencode(tx_buf->end, str);
}

void mc_write8(uint8_t value) {
    *tx_buf->end++ = value;
}

void mc_write16(uint16_t value) {
    mc_write8((uint8_t) (value >> 8));
    mc_write8((uint8_t) value);
}

void mc_write32(uint32_t value) {
    mc_write16((uint16_t) (value >> 8));
    mc_write16((uint16_t) value);
}

void mc_write64(uint64_t value) {
    mc_write32((uint32_t) (value >> 32));
    mc_write32((uint32_t) value);
}

uint32_t mc_read_var(net_buf_t* buf) {
    return mc_vardecode(buf->start, &buf->start);
}

char* mc_read_str(net_buf_t* buf) {
    return mc_strdecode(buf->start, &buf->start);
}

uint8_t mc_read8(net_buf_t* buf) {
    return *buf->start++;
}

uint16_t mc_read16(net_buf_t* buf) {
    return (uint16_t) mc_read8(buf) << 8 | mc_read8(buf);
}

uint32_t mc_read32(net_buf_t* buf) {
    return (uint32_t) mc_read16(buf) << 16 | mc_read16(buf);
}

uint64_t mc_read64(net_buf_t* buf) {
    return (uint64_t) mc_read32(buf) << 32 | mc_read32(buf);
}

void mc_set_compression(uint32_t threshold) {
    compression = threshold;
}

// --------------------- SERVERBOUND HANDSHAKE --------------------- //

void mc_send_handshake(tcp_conn_t* conn, uint32_t protocol, const char* host, uint16_t port, uint8_t state) {
    mc_write_var(protocol);
    mc_write_str(host);
    mc_write16(port);
    mc_write8(state);
    mc_wrap_send_packet(conn, 0x00);
}

// --------------------- SERVERBOUND STATUS --------------------- //

void mc_send_status_request(tcp_conn_t* conn) {
    mc_wrap_send_packet(conn, 0x00);
}

void mc_send_ping_request(tcp_conn_t* conn, uint64_t id) {
    mc_write64(id);
    mc_wrap_send_packet(conn, 0x01);
}

// --------------------- CLIENTBOUND STATUS --------------------- //

void mc_dealloc_status_response(void* data) {
    mc_status_response_t* packet = (mc_status_response_t*) data;
    free(packet->json);
    free(packet);
}

void* mc_decode_status_response(net_buf_t* buf) {
    mc_status_response_t* packet = malloc(sizeof(mc_status_response_t));
    packet->json = mc_read_str(buf);
    return packet;
}

void* mc_decode_ping_response(net_buf_t* buf) {
    mc_ping_response_t* packet = malloc(sizeof(mc_ping_response_t));
    packet->id = mc_read64(buf);
    return packet;
}

// --------------------- SERVERBOUND LOGIN --------------------- //

void mc_send_login_start(tcp_conn_t* conn, const char* name) {
    mc_write_str(name);
    mc_write8(0);
    mc_wrap_send_packet(conn, 0x00);
}

// --------------------- CLIENTBOUND LOGIN --------------------- //

void* mc_decode_set_compression(net_buf_t* buf) {
    mc_set_compression_t* packet = malloc(sizeof(mc_set_compression_t));
    packet->threshold = mc_read_var(buf);
    return packet;
}

// ------------------------------------------------------------- //

void mc_init() {
    if (tx_buf) {
        net_free_buf(tx_buf);
    }

    tx_buf = net_alloc_buf();
    rx_ptr = malloc(RX_BUF_SIZE);

    mc_status_decoders[0x00].decoder = mc_decode_status_response;
    mc_status_decoders[0x00].deallocator = mc_dealloc_status_response;
    mc_status_decoders[0x01].decoder = mc_decode_ping_response;
    mc_status_decoders[0x01].deallocator = free;

    mc_login_decoders[0x03].decoder = mc_decode_set_compression;
    mc_login_decoders[0x03].deallocator = free;
}