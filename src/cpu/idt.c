#include "idt.h"

void idt_encode_entry(idt_entry_t* entry, uint32_t offset, uint16_t selector, uint8_t dpl, uint8_t gate_type) {
    entry->offset1 = offset & 0xFFFF;
    entry->selector = selector;
    entry->zero = 0;
    entry->type_attributes = (1 << 7) | ((dpl & 0x3) << 5) | (gate_type & 0xF);
    entry->offset2 = offset >> 16;
}