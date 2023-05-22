#!/bin/bash
# TODO: Makefile
set -e
mkdir -p build
gcc -c vfs.c -o build/vfs.o -O2
gcc -c main.c -o build/main.o -O2
gcc build/vfs.o build/main.o -o build/misha.mkvfs