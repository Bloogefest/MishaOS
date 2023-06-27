OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS {
    . = 1M;
    kernel_virtual_start = .;
    kernel_physical_start = 0;

    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
        PROVIDE(bss_start = .);
    }

    kernel_virtual_end = .;
    kernel_physical_end = .;
}