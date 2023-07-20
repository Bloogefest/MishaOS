#!/bin/bash
# TODO: Makefile
set -e

mkdir -p build
i686-elf-gcc -c syscall.c -o build/syscall.o -std=gnu99 -ffreestanding -g -fno-omit-frame-pointer -Wall -O2 -Wextra -I../src
i686-elf-ar rcs libsyscall.a build/syscall.o