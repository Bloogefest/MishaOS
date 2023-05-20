#!/bin/bash
# TODO: Makefile
set -e
mkdir -p build
i686-elf-as src/boot.s -o build/boot.o
i686-elf-gcc -c src/kernel.c -o build/kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c src/terminal.c -o build/terminal.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-as src/gdt.s -o build/gdt_s.o
i686-elf-gcc -c src/gdt.c -o build/gdt.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-as src/idt.s -o build/idt_s.o
i686-elf-gcc -c src/idt.c -o build/idt.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c src/isrs.c -o build/isrs.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -mgeneral-regs-only -Wno-unused-parameter
i686-elf-gcc -c src/panic.c -o build/panic.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c src/string.c -o build/string.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c src/pic.c -o build/pic.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c src/io.c -o build/io.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -T linker.ld -o build/mishaos.bin -ffreestanding -O2 -nostdlib build/idt_s.o build/idt.o build/boot.o build/kernel.o build/terminal.o build/gdt_s.o build/gdt.o build/isrs.o build/panic.o build/string.o build/io.o build/pic.o -lgcc
mkdir -p build/iso/boot/grub
cp grub.cfg build/iso/boot/grub
cp -f build/mishaos.bin build/iso/boot
grub-mkrescue -o mishaos.iso build/iso