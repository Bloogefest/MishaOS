#include "pic.h"

#include <cpu/io.h>

void pic_master_eoi() {
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_slave_eoi() {
    outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_remap(uint8_t offset1, uint8_t offset2) {
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void pic_irq_set_mask_bit(uint8_t line) {
    uint16_t port;

    if (line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        line -= 8;
    }

    uint8_t value = inb(port) | (1 << line);
    outb(port, value);
}

void pic_irq_clear_mask_bit(uint8_t line) {
    uint16_t port;

    if (line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        line -= 8;
    }

    uint8_t value = inb(port) & ~(1 << line);
    outb(port, value);
}

void pic_irq_set_master_mask(uint8_t mask) {
    outb(PIC1_DATA, mask);
}

void pic_irq_set_slave_mask(uint8_t mask) {
    outb(PIC2_DATA, mask);
}