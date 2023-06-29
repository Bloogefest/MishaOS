#!/bin/bash
set -e

qemu-system-i386 -drive format=raw,file=mishaos_lgbt_boot.raw -net none