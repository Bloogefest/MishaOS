#!/bin/bash
# TODO: Makefile
mkdir -p build
i686-elf-as src/boot.s -o build/boot.o
i686-elf-gcc -c src/kernel.c -o build/kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c src/terminal.c -o build/terminal.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-as src/gdt.s -o build/gdt_s.o
i686-elf-gcc -c src/gdt.c -o build/gdt.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -T linker.ld -o build/mishaos.bin -ffreestanding -O2 -nostdlib build/boot.o build/kernel.o build/terminal.o build/gdt_s.o build/gdt.o -lgcc
mkdir -p build/iso/boot/grub
cp grub.cfg build/iso/boot/grub
cp -f build/mishaos.bin build/iso/boot
grub-mkrescue -o mishaos.iso build/iso