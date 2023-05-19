#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
} vga_color_t;

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

static inline uint8_t terminal_vga_color(vga_color_t fg, vga_color_t bg) {
    return fg | bg << 4;
}

static inline uint16_t terminal_vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t) color << 8 | c;
}

void terminal_init();
void terminal_set_color(uint8_t color);
void terminal_set_row(size_t row);
void terminal_set_column(size_t column);
void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y);
void terminal_clear_terminal();
void terminal_putchar(char c);
void terminal_put(const char* str, size_t size);
void terminal_putstring(const char* str);
