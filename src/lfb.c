#include "lfb.h"

uint8_t* linear_framebuffer;
size_t lfb_width;
size_t lfb_height;

void lfb_set_pixel(size_t x, size_t y, uint32_t color) {
    if (x >= lfb_width || y >= lfb_height) {
        return;
    }

    *((uint32_t*) linear_framebuffer + x + y * lfb_width) = color;
}

uint32_t lfb_get_pixel(size_t x, size_t y) {
    if (x >= lfb_width || y >= lfb_height) {
        return 0;
    }

    return *((uint32_t*) linear_framebuffer + x + y * lfb_width);
}