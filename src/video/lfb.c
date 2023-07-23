#include "lfb.h"

uint8_t* linear_framebuffer;
uint8_t* lfb_back_buffer = 0;
size_t lfb_width;
size_t lfb_height;

void lfb_set_pixel(size_t x, size_t y, uint32_t color) {
    if (x >= lfb_width || y >= lfb_height) {
        return;
    }

    uint32_t offset = x + y * lfb_width;
    *((uint32_t*) linear_framebuffer + offset) = color;
    if (lfb_back_buffer) {
        *((uint32_t *) lfb_back_buffer + offset) = color;
    }
}

uint32_t lfb_get_pixel(size_t x, size_t y) {
    if (x >= lfb_width || y >= lfb_height) {
        return 0;
    }

    if (lfb_back_buffer) {
        return *((uint32_t *) lfb_back_buffer + x + y * lfb_width);
    } else {
        return *((uint32_t *) linear_framebuffer + x + y * lfb_width);
    }
}

void lfb_set_back_buffer(void* buffer) {
    lfb_back_buffer = buffer;
}

uint8_t* lfb_get_back_buffer() {
    return lfb_back_buffer;
}