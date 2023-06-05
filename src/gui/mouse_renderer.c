#include "mouse_renderer.h"

#include "../mouse.h"
#include "../lfb.h"

static uint32_t screen_buffer[CURSOR_WIDTH * CURSOR_HEIGHT];
static uint8_t cursor_rendered = 0;

static uint32_t* cursor;

void mouse_fill_buffer() {
    for (uint32_t y = 0; y < CURSOR_HEIGHT; y++) {
        for (uint32_t x = 0; x < CURSOR_WIDTH; x++) {
            screen_buffer[y * CURSOR_WIDTH + x] = lfb_get_pixel(mouse_x + x, mouse_y + y);
        }
    }
}

void mouse_restore_buffer() {
    for (uint32_t y = 0; y < CURSOR_HEIGHT; y++) {
        for (uint32_t x = 0; x < CURSOR_WIDTH; x++) {
            lfb_set_pixel(last_mouse_x + x, last_mouse_y + y, screen_buffer[y * CURSOR_WIDTH + x]);
        }
    }
}

void mouse_render_cursor() {
    if (!mouse_dirty) {
        return;
    }

    mouse_dirty = 0;

    if (cursor_rendered) {
        mouse_restore_buffer();
    } else {
        mouse_x = 0;
        mouse_y = 0;
        cursor_rendered = 1;
    }

    mouse_fill_buffer();

    for (uint32_t y = 0; y < CURSOR_HEIGHT; y++) {
        for (uint32_t x = 0; x < CURSOR_WIDTH; x++) {
            uint32_t color = cursor[y * CURSOR_WIDTH + x];
            if ((color & 0xFF000000) == 0) {
                continue;
            }

            lfb_set_pixel(mouse_x + x, mouse_y + y, color);
        }
    }
}

void mouse_set_cursor(uint32_t* c) {
    cursor = c;
}