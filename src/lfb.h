#pragma once

#include <stdint.h>
#include <stddef.h>

extern uint8_t* linear_framebuffer;
extern size_t lfb_width;
extern size_t lfb_height;

void lfb_set_pixel(size_t x, size_t y, uint32_t color);
uint32_t lfb_get_pixel(size_t x, size_t y);