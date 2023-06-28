#!/bin/bash
# TODO: Makefile
set -e
rm -rf build
mkdir build

# Compile bootloader
printf "%%define bootloader_size 524288 ; (not actual size)\n" > build/constants.asm
i686-elf-gcc -c bootloader.c -o build/bootloader.o -O0 -std=gnu99 -ffreestanding -Wall
i686-elf-gcc -c memorymap.c -o build/memorymap.o -O0 -std=gnu99 -ffreestanding -Wall
i686-elf-gcc -c util.c -o build/util.o -O0 -std=gnu99 -ffreestanding -Wall
i686-elf-gcc -c iso9660.c -o build/iso9660.o -O0 -std=gnu99 -ffreestanding -Wall
nasm -felf bootloader.asm -o build/boot.bin
objects="build/boot.bin build/bootloader.o build/memorymap.o build/util.o build/iso9660.o"

# Link temporary bootloader
i686-elf-gcc -T linker.ld $objects -o build/bootloader.bin -nostdlib

# Determine bootloader size
bootloader_size=$(wc -c < build/bootloader.bin)
if [[ $(( bootloader_size % 512 )) != 0 ]]; then
  bootloader_size=$((bootloader_size + 512))
fi

printf "%%define bootloader_size %s\n" $bootloader_size > build/constants.asm

# Build bootloader
nasm -felf bootloader.asm -o build/boot.bin
i686-elf-gcc -T linker.ld $objects -o build/bootloader.bin -nostdlib

mkdir -p build/contents
cp -f build/bootloader.bin build/contents/bootloader.bin
cp -f ../../build/mishaos.bin build/contents/kernel.elf
cp -f ../../build/initrd.img build/contents/initrd.img
mkisofs -V MISHABOOT -o ../../mishaos_lgbt_boot.iso -b bootloader.bin -no-emul-boot build/contents/

printf "%%define READ_DISK\n" >> build/constants.asm
nasm -felf bootloader.asm -o build/boot.bin
i686-elf-gcc -T linker.ld $objects -o build/bootloader.bin -nostdlib
mkisofs -V MISHABOOT -o ../../mishaos_lgbt_boot.raw -b bootloader.bin -no-emul-boot build/contents/
dd if=build/bootloader.bin of=../../mishaos_lgbt_boot.raw conv=notrunc
