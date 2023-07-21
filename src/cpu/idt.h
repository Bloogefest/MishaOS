#pragma once

#include <cpu/gdt.h>

typedef struct idt_entry_s {
    uint16_t offset1;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attributes;
    uint16_t offset2;
} __attribute__((packed)) idt_entry_t;

void idt_encode_entry(idt_entry_t* entry, uint32_t offset, uint16_t selector, uint8_t dpl, uint8_t gate_type);
void idt_load(uint16_t limit, uint32_t base);