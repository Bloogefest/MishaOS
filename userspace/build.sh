#!/bin/bash
# TODO: Makefile
set -e

rm -rf build
rm -rf bin

mkdir -p build
mkdir -p bin
i686-elf-gcc -c hello.c -o build/hello.o -O2 -I../libsyscall -ffreestanding -std=gnu99
i686-elf-gcc ../libc/bin/crt0.o build/hello.o -o bin/hello -L../libsyscall -lsyscall -nostdlib