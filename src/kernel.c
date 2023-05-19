#include "terminal.h"
#include "gdt.h"

void kernel_main() {
    terminal_init();
    terminal_putstring("Optimizing fish...\n");

    gdt_entry_t gdt[6];
    gdt_encode(&gdt[0], 0, 0, 0, 0);
    gdt_encode(&gdt[1], 0, 0xFFFFF, 0x9A, 0xA);
    gdt_encode(&gdt[2], 0, 0xFFFFF, 0x92, 0xC);
    gdt_encode(&gdt[3], 0, 0xFFFFF, 0xFA, 0xA);
    gdt_encode(&gdt[4], 0, 0xFFFFF, 0xF2, 0xC);
    gdt_encode(&gdt[5], (uint32_t) &gdt[5], sizeof(gdt[5]), 0x89, 0);
    gdt_load(sizeof(gdt) - 1, (uint32_t) &gdt);

    terminal_putstring("Done. MishaOS loaded.");
}