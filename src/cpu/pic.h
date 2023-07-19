#pragma once

#include <stdint.h>

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)

#define PIC_EOI 0x20

#define ICW1_ICW4 0x01
#define ICW1_SINGLE 0x02
#define ICW1_INTERNAL4 0x04
#define ICW1_LEVEL 0x08
#define ICW1_INIT 0x10

#define ICW4_8086 0x01
#define ICW4_AUTO 0x02
#define ICW4_BUF_SLAVE 0x08
#define ICW4_BUF_MASTER 0x0C
#define ICW4_SFNM 0x10

void pic_master_eoi();
void pic_slave_eoi();

void pic_remap(uint8_t offset1, uint8_t offset2);

void pic_irq_set_mask_bit(uint8_t line);
void pic_irq_clear_mask_bit(uint8_t line);
void pic_irq_set_master_mask(uint8_t mask);
void pic_irq_set_slave_mask(uint8_t mask);