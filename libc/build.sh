#!/bin/bash
# TODO: Makefile
set -e

mkdir -p build
mkdir -p bin

i686-elf-as crt0.s -o bin/crt0.o