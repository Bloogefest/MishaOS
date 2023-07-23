#include "gdt.h"

#include <cpu/tss.h>
#include <lib/string.h>

tss_entry_t tss_entry;

void gdt_encode_entry(gdt_entry_t* entry, uint32_t base, uint32_t limit, uint8_t access_byte, uint8_t flags) {
    uint8_t* target = (uint8_t*) entry;
    target[0] = limit & 0xFF;
    target[1] = (limit >> 8) & 0xFF;
    target[6] = (limit >> 16) & 0x0F;
    target[2] = base & 0xFF;
    target[3] = (base >> 8) & 0xFF;
    target[4] = (base >> 16) & 0xFF;
    target[7] = (base >> 24) & 0xFF;
    target[5] = access_byte;
    target[6] |= (flags << 4);
}

void tss_encode_entry(gdt_entry_t* entry, uint16_t ss0, uint32_t esp0) {
    uintptr_t base = (uintptr_t) &tss_entry;
    uintptr_t limit = base + sizeof(tss_entry);

    gdt_encode_entry(entry, base, limit, 0xE9, 0x00);

    memset(&tss_entry, 0, sizeof(tss_entry));

    tss_entry.ss0 = ss0;
    tss_entry.esp0 = esp0;
    tss_entry.cs = 0x0B;
    tss_entry.ss = 0x13;
    tss_entry.ds = 0x13;
    tss_entry.es = 0x13;
    tss_entry.fs = 0x13;
    tss_entry.gs = 0x13;
    tss_entry.iopb = sizeof(tss_entry);
}

void set_kernel_stack(uintptr_t stack) {
    tss_entry.esp0 = stack;
}