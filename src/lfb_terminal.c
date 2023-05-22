#include "lfb_terminal.h"
#include "lfb.h"
#include "vga_terminal.h"

terminal_t lfb_terminal = {.init = lfb_terminal_init, .putchar = lfb_terminal_putchar, .clear = lfb_terminal_clear_terminal, .rows = 768 / 16, .columns = 1024 / 8};

static psf_font_t* font;

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

void lfb_copy_from_vga() {
    uint32_t row = terminal_row;
    uint32_t column = terminal_column;

    terminal_row = 0;
    terminal_column = 0;
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            uint8_t character = vga_get_stack_buffer()[y * VGA_WIDTH + x];
            if (!character) {
                break;
            }

            lfb_terminal_putchar(character);
        }
        lfb_terminal_putchar('\n');
    }

    terminal_row = row;
    terminal_column = column;
}

void lfb_terminal_set_font(psf_font_t* f) {
    font = f;
}

void lfb_terminal_init() {
    terminal_set_row(0);
    terminal_set_column(0);
}

void lfb_terminal_putchar(char ch) {
    switch (ch) {
        case '\n': {
            terminal_column = 0;
            terminal_row++;
            return;
        }
    }

    char* face = (char*) font->buffer + (ch * font->header.character_size);

    for (uint64_t j = 0; j < 16; j++) {
        for (uint64_t i = 0; i < 8; i++) {
            uint32_t nx = (terminal_column * 8) + i;
            uint32_t ny = (terminal_row * 16) + j;

            uint32_t color = (*face & (0b10000000 >> i)) > 0
                             ? 0xFFFFFFFF : 0x00000000;

            *((uint32_t*) linear_framebuffer + nx + ny * lfb_width) = color;
        }

        face++;
    }

    terminal_column++;
}

void lfb_terminal_clear_terminal() {
    for (size_t y = 0; y < lfb_height; y++) {
        for (size_t x = 0; x < lfb_width; x++) {
            *((uint32_t*) linear_framebuffer + x + y * lfb_width) = 0;
        }
    }
}