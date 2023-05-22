#!/bin/bash
# TODO: Makefile
set -e

cc_flags="-std=gnu99 -ffreestanding -O2 -Wall -Wextra -Imishavfs -Isrc"

# Build sources
mkdir -p build
i686-elf-as src/boot.s -o build/boot.o
i686-elf-gcc -c src/kernel.c -o build/kernel.o $cc_flags
i686-elf-gcc -c src/terminal.c -o build/terminal.o $cc_flags
i686-elf-as src/gdt.s -o build/gdt_s.o
i686-elf-gcc -c src/gdt.c -o build/gdt.o $cc_flags
i686-elf-as src/idt.s -o build/idt_s.o
i686-elf-gcc -c src/idt.c -o build/idt.o $cc_flags
i686-elf-gcc -c src/isrs.c -o build/isrs.o $cc_flags -mgeneral-regs-only -Wno-unused-parameter
i686-elf-gcc -c src/panic.c -o build/panic.o $cc_flags
i686-elf-gcc -c src/string.c -o build/string.o $cc_flags
i686-elf-gcc -c src/pic.c -o build/pic.o $cc_flags
i686-elf-gcc -c src/io.c -o build/io.o $cc_flags
i686-elf-gcc -c src/acpi.c -o build/acpi.o $cc_flags
i686-elf-gcc -c src/stdlib.c -o build/stdlib.o $cc_flags
i686-elf-gcc -c src/pci.c -o build/pci.o $cc_flags
i686-elf-gcc -c src/driver.c -o build/driver.o $cc_flags
i686-elf-gcc -c src/ide.c -o build/ide.o $cc_flags
i686-elf-gcc -c mishavfs/vfs.c -o build/vfs.o $cc_flags
i686-elf-gcc -T linker.ld -o build/mishaos.bin -ffreestanding -O2 -nostdlib build/idt_s.o build/idt.o build/boot.o build/kernel.o build/terminal.o build/gdt_s.o build/gdt.o build/isrs.o build/panic.o build/string.o build/io.o build/pic.o build/acpi.o build/stdlib.o build/pci.o build/driver.o build/ide.o build/vfs.o -lgcc

# Build mishavfs
pushd mishavfs
./build.sh
popd

# Create initrd
rm -f build/initrd.img
mishavfs/build/misha.mkvfs --label=INITRD initrd build/initrd.img

# Create bootable image
mkdir -p build/iso/boot/grub
cp -f grub.cfg build/iso/boot/grub
cp -f build/mishaos.bin build/iso/boot
mv -f build/initrd.img build/iso/boot
grub-mkrescue -o mishaos.iso build/iso