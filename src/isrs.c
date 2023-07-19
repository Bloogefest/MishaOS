#include "isrs.h"
#include "kprintf.h"
#include "panic.h"
#include "io.h"
#include "pic.h"
#include "mouse.h"
#include "pit.h"

#define PERIPHERAL_HANDLER(id)                                   \
    __attribute__((interrupt))                                   \
    void peripheral_handler##id(struct interrupt_frame* frame) { \
        irq_handler_t handler = peripheral_isrs[id];             \
        if (handler) {                                           \
            handler(frame);                                      \
        } else {                                                 \
            pic_master_eoi();                                    \
        }                                                        \
    }                                                            \

irq_handler_t peripheral_isrs[3];

__attribute__((interrupt))
void general_protection_fault_isr(struct interrupt_frame* frame) {
    asm("cli");
    panic("General Protection Fault");
}

__attribute__((interrupt))
void double_fault_isr(struct interrupt_frame* frame) {
    asm("cli");
    panic("Double Fault");
}

__attribute__((interrupt))
void page_fault_isr(struct interrupt_frame* frame) {
    asm("cli");
    uint32_t fault_addr;
    asm volatile("mov %%cr2, %0" : "=r"(fault_addr));
    kprintf("Page Fault at 0x%lx:\n Present: %d\n R/W: %d\n User: %d\n",
            fault_addr, !(frame->err_code & 0x01), !!(frame->err_code & 0x02), !!(frame->err_code & 0x04));
    panic("Page Fault");
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
void keyboard_isr(struct interrupt_frame* frame) {
    asm("cli");
    uint8_t scancode = inb(0x60);
    switch (scancode) {
        case 0x1C: { // Enter pressed
            putchar('\n');
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
                column = terminal_get_columns() - 1;
                if (--row == (size_t) -1) {
                    row = terminal_get_rows() - 1;
                }
            }

            terminal_set_column(column);
            terminal_set_row(row);
            putchar(' ');
            terminal_set_column(column);
            break;
        }

        default: {
            if (scancode >= sizeof(SCAN_CODE_SET)) {
                break;
            }

            char c = SCAN_CODE_SET[scancode];
            uint8_t uppercase = CAPS_LOCK ^ SHIFT;
            if (c) {
                putchar(c - uppercase * 32);
            }

            break;
        }
    }

    pic_master_eoi();
    asm("sti");
}

__attribute__((interrupt))
void ps2_mouse_isr(struct interrupt_frame* frame) {
    asm("cli");
    mouse_read_packet();
    pic_slave_eoi();
    asm("sti");
}

__attribute__((interrupt))
void pit_isr(struct interrupt_frame* frame) {
    asm("cli");
    pit_tick();
    pic_master_eoi();
    asm("sti");
}

PERIPHERAL_HANDLER(0)
PERIPHERAL_HANDLER(1)
PERIPHERAL_HANDLER(2)