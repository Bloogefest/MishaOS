#include "terminal.h"
#include "gdt.h"
#include "tss.h"
#include "idt.h"
#include "isrs.h"

void kernel_main() {
    terminal_init();
    terminal_putstring("Optimizing fish...\n");

    gdt_entry_t gdt[6];
    tss_entry_t tss;

    gdt_encode_entry(&gdt[0], 0, 0, 0, 0);
    gdt_encode_entry(&gdt[1], 0, 0xFFFFF, 0x9A, 0xC);
    gdt_encode_entry(&gdt[2], 0, 0xFFFFF, 0x92, 0xC);
    gdt_encode_entry(&gdt[3], 0, 0xFFFFF, 0xFA, 0xC);
    gdt_encode_entry(&gdt[4], 0, 0xFFFFF, 0xF2, 0xC);
    gdt_encode_entry(&gdt[5], (uint32_t) &tss, sizeof(tss), 0x89, 0x0);
    gdt_load(sizeof(gdt) - 1, (uint32_t) &gdt);

    idt_entry_t idt[256];
    idt_encode_entry(&idt[49], (uint32_t) interrupt_handler, 0x08, 0, 0xE);
    idt_load(sizeof(idt) - 1, (uint32_t) &idt);

    asm("int $49");

    terminal_putstring("Done. MishaOS loaded.");

    for (;;) {
        asm("hlt");
    }
}