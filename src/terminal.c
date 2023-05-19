#include "terminal.h"
#include "string.h"

static uint8_t terminal_color;
static uint16_t* terminal_buffer;
static size_t terminal_row;
static size_t terminal_column;

void terminal_init() {
    terminal_buffer = (uint16_t*) 0xB8000;
    terminal_color = terminal_vga_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_row = 0;
    terminal_column = 0;
}

void terminal_set_color(uint8_t color) {
    terminal_color = color;
}

void terminal_set_row(size_t row) {
    terminal_row = row;
}

void terminal_set_column(size_t column) {
    terminal_column = column;
}

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) {
    terminal_buffer[y * VGA_WIDTH + x] = terminal_vga_entry(c, color);
}

void terminal_clear_terminal() {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_putentryat(' ', terminal_color, x, y);
        }
    }
}

void terminal_putchar(char c) {
    switch (c) {
        case '\n': {
            terminal_column = 0;
            terminal_row++;
            break;
        }

        default: {
            terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
            if (++terminal_column == VGA_WIDTH) {
                terminal_column = 0;
                if (++terminal_row == VGA_HEIGHT) {
                    terminal_row = 0;
                }
            }
        }
    }
}

void terminal_put(const char* str, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(str[i]);
    }
}

void terminal_putstring(const char* str) {
    terminal_put(str, strlen(str));
}