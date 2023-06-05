#pragma once

#include <stdint.h>
#include <stddef.h>

#define CURSOR_WIDTH 16
#define CURSOR_HEIGHT 20

extern uint8_t mouse_buttons[5];
extern size_t mouse_x;
extern size_t mouse_y;
extern size_t last_mouse_x;
extern size_t last_mouse_y;
extern int mouse_scroll;
extern uint8_t mouse_dirty;

void mouse_wait_write(uint8_t port, uint8_t value);
void mouse_write(uint8_t value);
uint8_t mouse_read();
void mouse_read_packet();
void mouse_handle_packet();
void mouse_init();