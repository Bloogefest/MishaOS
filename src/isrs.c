#include "isrs.h"
#include "terminal.h"
#include "panic.h"
#include "io.h"
#include "pic.h"

__attribute__((interrupt))
void general_protection_fault_isr(struct interrupt_frame* frame) {
    panic("General Protection Fault");
}

__attribute__((interrupt))
void double_fault_isr(struct interrupt_frame* frame) {
    panic("Double Fault");
}

static const char SCAN_CODE_SET[] = {
        0x0, 0x0, '1', '2',
        '3', '4', '5', '6',
        '7', '8', '9', '0',
        '-', '=', 0x0, 0x0,
        'q', 'w', 'e', 'r',
        't', 'y', 'u', 'i',
        'o', 'p', '[', ']',
        0x0, 0x0, 'a', 's',
        'd', 'f', 'g', 'h',
        'j', 'k', 'l', ';',
        '\'','`', 0x0, '\\',
        'z', 'x', 'c', 'v',
        'b', 'n', 'm', ',',
        '.', '/', 0x0, '*',
        0x0, ' ', 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, '7',
        '8', '9', '-', '4',
        '5', '6', '+', '1',
        '2', '3', '0', '.'
};

static uint8_t CAPS_LOCK;
static uint8_t SHIFT;

__attribute__((interrupt))
void master_mask_port_isr(struct interrupt_frame* frame) {
    uint8_t scancode = inb(0x60);
    switch (scancode) {
        case 0x1C: { // Enter pressed
            terminal_putchar('\n');
            break;
        }

        case 0x2A: { // Left Shift pressed
            SHIFT = 1;
            break;
        }

        case 0xAA: { // Left Shift released
            SHIFT = 0;
            break;
        }

        case 0x3A: { // Caps Lock pressed
            CAPS_LOCK ^= 1;
            break;
        }

        case 0x0E: { // Backspace pressed
            size_t column = terminal_get_column();
            size_t row = terminal_get_row();
            if (--column == (size_t) -1) {
                column = VGA_WIDTH - 1;
                if (--row == (size_t) -1) {
                    row = VGA_HEIGHT - 1;
                }
            }

            terminal_putentryat(' ', terminal_get_color(), column, row);
            terminal_set_column(column);
            terminal_set_row(row);
            break;
        }

        default: {
            if (scancode >= sizeof(SCAN_CODE_SET)) {
                break;
            }

            char c = SCAN_CODE_SET[scancode];
            uint8_t uppercase = CAPS_LOCK ^ SHIFT;
            if (c) {
                terminal_putchar(c - uppercase * 32);
            }

            break;
        }
    }

    pic_master_eoi();
}