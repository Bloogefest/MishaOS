#include "kernel.h"
#include "terminal.h"
#include "vga_terminal.h"
#include "lfb_terminal.h"
#include "lfb.h"
#include "tga.h"
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
#include "mouse.h"
#include "gpd.h"
#include "pit.h"
#include "paging.h"
#include "heap.h"
#include "net/net.h"
#include "gui/mouse_renderer.h"
#include "gui/gui.h"
#include "mc/f3f5.h"

#include "../mishavfs/vfs.h"

void kernel_main(kernel_meminfo_t meminfo, struct multiboot* multiboot, uint32_t multiboot_msg, uint32_t esp) {
    terminal = vga_terminal;
    terminal_init();
    terminal_putstring("Optimizing fish...\n");
    char num[20];

    uint32_t cursor_buffer[CURSOR_WIDTH * CURSOR_HEIGHT + 2];

    uint32_t module_start = 0;
    uint32_t module_end = 0;
    vfs_filesystem_t initrd;

    if (multiboot_msg == MULTIBOOT_EAX_MAGIC) {
        terminal_putstring("Multiboot structure is valid.\n");
        if (!(multiboot->flags >> 6 & 0x1)) {
            panic("Invalid memory map");
        }

        if (multiboot->mods_count == 0) {
            panic("Initial ramdisk not found.");
        }

        module_start = *(uint32_t*) multiboot->mods_addr;
        module_end = *(uint32_t*) (multiboot->mods_addr + 4);
        itoa(module_end - module_start, num, 10);
        terminal_putstring("Found initrd: ");
        terminal_putstring(num);
        terminal_putstring(" bytes\n");

        terminal_putstring("Loading initrd... \n");

        vfs_read_filesystem(&initrd, (uint8_t*) module_start);
        vfs_entry_t* test_entry = vfs_find_entry(&initrd, ".initrd_test");
        if (!test_entry || memcmp("optimizedfish", vfs_file_content(&initrd, test_entry, 0), test_entry->size)) {
            panic("Failed to verify initrd");
        }

        vbe_info_t* vbe_info = ((vbe_info_t*) (multiboot->vbe_mode_info));
        linear_framebuffer = (uint8_t*) vbe_info->physbase;
        lfb_width = vbe_info->x_res;
        lfb_height = vbe_info->y_res;

        terminal_putstring("Initialized linear framebuffer.\n");
    } else {
        panic("Invalid multiboot header.");
    }

    vfs_entry_t* logo = vfs_find_entry(&initrd, "logo.tga");
    tga_parse((uint32_t*) linear_framebuffer - 2 /* i'm not sure if this is safe */, vfs_file_content(&initrd, logo, 0), logo->size);

    vfs_entry_t* font = vfs_find_entry(&initrd, "zap-vga16.psf");
    psf_font_t* psf_font = psf_load_font(vfs_file_content(&initrd, font, 0));

    terminal = lfb_terminal;
    terminal_init();
    lfb_terminal_set_font(psf_font);
    lfb_copy_from_vga();

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
    idt_encode_entry(&idt[0x0E], (uint32_t) page_fault_isr, 0x08, 0, 0xE);
    idt_encode_entry(&idt[0x20], (uint32_t) pit_isr, 0x08, 0, 0xE);
    idt_encode_entry(&idt[0x21], (uint32_t) keyboard_isr, 0x08, 0, 0xE);
    idt_encode_entry(&idt[0x29], (uint32_t) peripheral_handler0, 0x08, 0, 0xE);
    idt_encode_entry(&idt[0x2A], (uint32_t) peripheral_handler1, 0x08, 0, 0xE);
    idt_encode_entry(&idt[0x2B], (uint32_t) peripheral_handler2, 0x08, 0, 0xE);
    idt_encode_entry(&idt[0x2C], (uint32_t) ps2_mouse_isr, 0x08, 0, 0xE);
    idt_load(sizeof(idt) - 1, (uint32_t) &idt);

    terminal_putstring("Initializing paging...\n");
    pfa_read_memory_map(&pfa, multiboot, &meminfo, module_start, module_end);
    pde_init(&page_directory);

    multiboot_memory_map_t* entry = (multiboot_memory_map_t*) multiboot->mmap_addr;
    while ((uint32_t) entry < multiboot->mmap_addr + multiboot->mmap_length) {
        for (uint32_t i = (uint32_t) entry->addr & 0xFFFF; i <= (uint32_t) entry->len; i += 0x1000) {
            pde_map_memory(&page_directory, &pfa, (void*) i, (void*) i);
        }

        entry = (multiboot_memory_map_t*) (((uint32_t) entry) + entry->size + sizeof(entry->size));
    }

    for (uint32_t i = (uint32_t) linear_framebuffer; i < (uint32_t) linear_framebuffer + lfb_height * lfb_width * 4 + 0x1000; i += 0x1000) {
        pde_map_memory(&page_directory, &pfa, (void*) i, (void*) i);
    }

    enable_paging(&page_directory);

    terminal_putstring("Initializing heap...\n");
    heap_init((void*) 2147483648U, 0x10); // TODO: 64-bit kernel for larger address space

    terminal_putstring("Remapping PIC...\n");
    pic_remap(0x20, 0x28);

    terminal_putstring("Initializing mouse...\n");
    vfs_entry_t* cursors = vfs_find_entry(&initrd, "cursors");
    vfs_entry_t* cursor = vfs_find_entry_in(&initrd, cursors, "center_ptr.tga");
    tga_parse(cursor_buffer, vfs_file_content(&initrd, cursor, 0), cursor->size);
    if (cursor_buffer[0] != CURSOR_WIDTH || cursor_buffer[1] != CURSOR_HEIGHT) {
        panic("Invalid cursor size.");
    }

    mouse_set_cursor(cursor_buffer + 2);
    mouse_init();

    terminal_putstring("Initializing PIT...\n");
    pit_set_phase(1000);

    terminal_putstring("Enabling IRQs...\n");
    pic_irq_set_master_mask(0b11111000);
    pic_irq_set_slave_mask(0b11100001);

    asm("sti");

    terminal_putstring("Initializing PCI...\n");
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

    terminal_putstring("Initializing networking...\n");
    net_post_init = f3f5_connect;
    net_init();

    terminal_putstring("Done. MishaOS loaded.\n");

    for (;;) {
        mouse_handle_packet();
        gui_redraw();
        net_poll();
    }
}