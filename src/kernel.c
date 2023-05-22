#include "terminal.h"
#include "gdt.h"
#include "tss.h"
#include "idt.h"
#include "isrs.h"
#include "pic.h"
#include "acpi.h"
#include "panic.h"
#include "pci.h"
#include "multiboot.h"
#include "ide.h"
#include "stdlib.h"
#include "string.h"
#include "../mishavfs/vfs.h"

void kernel_main(struct multiboot* multiboot, uint32_t multiboot_msg, uint32_t esp) {
    terminal_init();
    terminal_putstring("Optimizing fish...\n");

    if (multiboot_msg == MULTIBOOT_EAX_MAGIC) {
        terminal_putstring("Multiboot structure is valid.\n");
        if (multiboot->mods_count == 0) {
            panic("Initial ramdisk not found.");
        }

        uint32_t module_start = *(uint32_t *) multiboot->mods_addr;
        uint32_t module_end = *(uint32_t *) (multiboot->mods_addr + 4);
        char num[11];
        itoa(module_end - module_start, num, 10);
        terminal_putstring("Found initrd: ");
        terminal_putstring(num);
        terminal_putstring(" bytes\n");

        terminal_putstring("Loading initrd... ");

        vfs_filesystem_t initrd;
        vfs_read_filesystem(&initrd, (uint8_t*) module_start);
        vfs_entry_t* test_entry = vfs_find_entry(&initrd, ".initrd_test");
        if (!test_entry || _strcmp("optimizedfish", vfs_file_content(&initrd, test_entry, 0), test_entry->size)) {
            panic("Failed to verify initrd");
        }

        terminal_putstring("done.\n");
    } else {
        panic("Invalid multiboot header.");
    }

    rsdp_t* rsdp = rsdp_locate();
    if (!rsdp) {
        panic("RSDP not found");
    }

    terminal_putstring("Found RDSP [");
    terminal_put((const char*) &rsdp->oem_id, 6);
    terminal_putstring("]");
    if (rsdp->revision == 0) {
        terminal_putstring(" version 1.0\n");
        acpi_parse_rsdt((sdt_header_t*) rsdp->rsdt_address);
    } else if (rsdp->revision == 2) {
        terminal_putstring(" version 2.0\n");
        rsdp2_t* rsdp2 = (rsdp2_t*) rsdp;
        if (rsdp2->xsdt_address) {
            acpi_parse_xsdt((sdt_header_t*) rsdp2->xsdt_address);
        } else {
            acpi_parse_rsdt((sdt_header_t*) rsdp->rsdt_address);
        }
    } else {
        panic("Unsupported ACPI version");
    }

    gdt_entry_t gdt[6];
    tss_entry_t tss;

    terminal_putstring("Loading GDT...\n");
    gdt_encode_entry(&gdt[0], 0, 0, 0, 0);
    gdt_encode_entry(&gdt[1], 0, 0xFFFFF, 0x9A, 0xC);
    gdt_encode_entry(&gdt[2], 0, 0xFFFFF, 0x92, 0xC);
    gdt_encode_entry(&gdt[3], 0, 0xFFFFF, 0xFA, 0xC);
    gdt_encode_entry(&gdt[4], 0, 0xFFFFF, 0xF2, 0xC);
    gdt_encode_entry(&gdt[5], (uint32_t) &tss, sizeof(tss), 0x89, 0x0);
    gdt_load(sizeof(gdt) - 1, (uint32_t) &gdt);

    terminal_putstring("Loading IDT...\n");
    idt_entry_t idt[256];
    idt_encode_entry(&idt[0x08], (uint32_t) double_fault_isr, 0x08, 0, 0xE);
    idt_encode_entry(&idt[0x0D], (uint32_t) general_protection_fault_isr, 0x08, 0, 0xE);
    idt_encode_entry(&idt[0x21], (uint32_t) master_mask_port_isr, 0x08, 0, 0xE);
    idt_load(sizeof(idt) - 1, (uint32_t) &idt);

    terminal_putstring("Remapping PIC...\n");
    pic_remap(0x20, 0x28);

    pic_irq_set_master_mask(0b11111101);
    pic_irq_set_slave_mask(0b11111111);

    asm("sti");

    for (uint32_t bus = 0; bus < 256; ++bus) {
        for (uint32_t dev = 0; dev < 32; ++dev) {
            uint32_t base_id = pci_get_id(bus, dev, 0);
            uint8_t header_type = pci_read8(base_id, PCI_CONFIG_HEADER_TYPE);
            uint32_t func_count = header_type & PCI_TYPE_MULTIFUNC ? 8 : 1;

            for (uint32_t func = 0; func < func_count; ++func) {
                pci_visit(bus, dev, func);
            }
        }
    }

    ide_init(0x1F0, 0x3F6, 0x170, 0x376, 0x000);

    terminal_putstring("Done. MishaOS loaded.\n");

    for (;;) {
        asm("hlt");
    }
}