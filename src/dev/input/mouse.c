#include "mouse.h"

#include <cpu/io.h>
#include <lib/kprintf.h>
#include <lib/stdlib.h>
#include <video/lfb.h>

static uint8_t wheel = 0;
static uint8_t buttons = 0;

uint8_t mouse_buttons[5];
size_t mouse_x = 0;
size_t mouse_y = 0;
size_t last_mouse_x = 0;
size_t last_mouse_y = 0;
int mouse_scroll = 0;
uint8_t mouse_dirty = 0;

static uint8_t packet_bytes = 0;
static uint8_t packet_index = 0;
static uint8_t packet[4];

void mouse_wait_write(uint8_t port, uint8_t value) {
    for (uint32_t timeout = 100000; timeout > 0; --timeout) {
        if (!(inb(0x64) & 0x02)) {
            break;
        }
    }

    outb(port, value);
}

void mouse_write(uint8_t value) {
    mouse_wait_write(0x64, 0xD4);
    mouse_wait_write(0x60, value);
    mouse_read(); // TODO: Verify keyboard ack?
}

uint8_t mouse_read() {
    for (uint32_t timeout = 100000; timeout > 0; --timeout) {
        if (inb(0x64) & 0x01) {
            break;
        }
    }

    return inb(0x60);
}

void mouse_read_packet() {
    uint8_t data = inb(0x60);
    if (packet_index >= packet_bytes) {
        return;
    }

    if (packet_index == 0 && !(data & 0x08)) {
        return;
    }

    packet[packet_index++] = data;
}

void mouse_handle_packet() {
    if (packet_index < packet_bytes) {
        return;
    }

    mouse_buttons[0] = packet[0] & 0x01;
    mouse_buttons[1] = (packet[0] & 0x02) >> 1;
    mouse_buttons[2] = (packet[0] & 0x04) >> 2;
    mouse_buttons[3] = (packet[3] & 0x10) >> 4;
    mouse_buttons[4] = (packet[3] & 0x20) >> 5;

    last_mouse_x = mouse_x;
    last_mouse_y = mouse_y;

    mouse_x += (packet[0] & 0x10) ? packet[1] - 256 : packet[1];
    mouse_y -= (packet[0] & 0x20) ? packet[2] - 256 : packet[2];

    if (packet[0] & 0x40) {
        mouse_x += (packet[0] & 0x10) ? -255 : 255;
    }

    if (packet[0] & 0x80) {
        mouse_y += (packet[0] & 0x20) ? 255 : -255;
    }

    if (mouse_x >= SIZE_MAX - UINT8_MAX) {
        mouse_x = 0;
    }

    if (mouse_y >= SIZE_MAX - UINT8_MAX) {
        mouse_y = 0;
    }

    if (mouse_x >= lfb_width) {
        mouse_x = lfb_width - 1;
    }

    if (mouse_y >= lfb_height) {
        mouse_y = lfb_height - 1;
    }

    packet_index = 0;
    mouse_dirty = 1;
}

void mouse_init() {
    mouse_wait_write(0x64, 0xA8);

    mouse_wait_write(0x64, 0x20);
    uint8_t status = mouse_read();
    status |= 0b10;
    mouse_wait_write(0x64, 0x60);
    mouse_wait_write(0x60, status);

    mouse_wait_write(0x64, 0xA9);
    uint8_t if_resp = mouse_read();
    kprintf("Interface response: 0x%x\n", if_resp);

    mouse_write(0xF2);
    uint8_t id = mouse_read();
    kprintf("ID response: 0x%x\n", id);

    mouse_write(0xF3);
    mouse_write(0xC8);
    mouse_write(0xF3);
    mouse_write(0x64);
    mouse_write(0xF3);
    mouse_write(0x50);

    mouse_write(0xF2);
    wheel = mouse_read();
    kprintf("Wheel mouse: 0x%x\n", wheel);

    mouse_write(0xF3);
    mouse_write(0xC8);
    mouse_write(0xF3);
    mouse_write(0xC8);
    mouse_write(0xF3);
    mouse_write(0x50);

    mouse_write(0xF2);
    buttons = mouse_read();
    kprintf("Ext buttons: 0x%x\n", buttons);

    mouse_write(0xF6);
    mouse_write(0xE6);
    mouse_write(0xF4);
    mouse_write(0xF3);
    mouse_write(0x64);
    mouse_write(0xE8);
    mouse_write(0x03);

    mouse_write(0xF4);
    uint8_t enable_resp = mouse_read();
    kprintf("Enable response: 0x%x\n", enable_resp);

    outb(0xA1, inb(0xA1) & ~0x10);

    packet_bytes = wheel == 0x03 || buttons == 0x04 ? 4 : 3;
}