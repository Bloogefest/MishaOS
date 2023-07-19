#!/bin/bash
# TODO: Makefile
set -e

cc_flags="-std=gnu99 -ffreestanding -g -fno-omit-frame-pointer -Wall -O2 -Wextra -Imishavfs -Isrc -Wno-unused-parameter -DMISHAOS -DMISHAOS_KERNEL"

# Build kernel
rm -rf build
mkdir -p build
mkdir -p build/cpu
mkdir -p build/dev
mkdir -p build/dev/input
mkdir -p build/dev/net
mkdir -p build/dev/storage
mkdir -p build/dev/usb
mkdir -p build/lib
#mkdir -p build/mc
mkdir -p build/misc
mkdir -p build/net
mkdir -p build/sys
mkdir -p build/video
mkdir -p build/vfs

i686-elf-as     src/boot.s                 -o build/boot.o
i686-elf-gcc -c src/kernel.c               -o build/kernel.o               $cc_flags
i686-elf-gcc -c src/cpu/acpi.c             -o build/cpu/acpi.o             $cc_flags
i686-elf-gcc -c src/cpu/gdt.c              -o build/cpu/gdt.o              $cc_flags
i686-elf-as     src/cpu/gdt.s              -o build/cpu/gdt_s.o
i686-elf-gcc -c src/cpu/idt.c              -o build/cpu/idt.o              $cc_flags
i686-elf-as     src/cpu/idt.s              -o build/cpu/idt_s.o
i686-elf-gcc -c src/cpu/io.c               -o build/cpu/io.o               $cc_flags
i686-elf-gcc -c src/cpu/paging.c           -o build/cpu/paging.o           $cc_flags -O0
i686-elf-gcc -c src/cpu/pic.c              -o build/cpu/pic.o              $cc_flags
i686-elf-gcc -c src/dev/input/mouse.c      -o build/dev/input/mouse.o      $cc_flags
i686-elf-gcc -c src/dev/net/intel.c        -o build/dev/net/intel.o        $cc_flags
i686-elf-gcc -c src/dev/net/rtl8139.c      -o build/dev/net/rtl8139.o      $cc_flags
i686-elf-gcc -c src/dev/storage/ide.c      -o build/dev/storage/ide.o      $cc_flags
#i686-elf-gcc -c src/dev/usb/controller.c   -o build/dev/usb/controller.o   $cc_flags
#i686-elf-gcc -c src/dev/usb/desc.c         -o build/dev/usb/desc.o         $cc_flags
#i686-elf-gcc -c src/dev/usb/dev.c          -o build/dev/usb/dev.o          $cc_flags
#i686-elf-gcc -c src/dev/usb/driver.c       -o build/dev/usb/driver.o       $cc_flags
#i686-elf-gcc -c src/dev/usb/ehci.c         -o build/dev/usb/ehci.o         $cc_flags
#i686-elf-gcc -c src/dev/usb/uhci.c         -o build/dev/usb/uhci.o         $cc_flags
#i686-elf-gcc -c src/dev/usb/usb.c          -o build/dev/usb/usb.o          $cc_flags
i686-elf-gcc -c src/dev/driver.c           -o build/dev/driver.o           $cc_flags
i686-elf-gcc -c src/dev/pci.c              -o build/dev/pci.o              $cc_flags
i686-elf-gcc -c src/lib/ctype.c            -o build/lib/ctype.o            $cc_flags
i686-elf-gcc -c src/lib/kprintf.c          -o build/lib/kprintf.o          $cc_flags
i686-elf-gcc -c src/lib/sleep.c            -o build/lib/sleep.o            $cc_flags
i686-elf-gcc -c src/lib/stdlib.c           -o build/lib/stdlib.o           $cc_flags
i686-elf-gcc -c src/lib/string.c           -o build/lib/string.o           $cc_flags
i686-elf-gcc -c src/lib/terminal.c         -o build/lib/terminal.o         $cc_flags
i686-elf-gcc -c src/lib/tga.c              -o build/lib/tga.o              $cc_flags
i686-elf-gcc -c src/lib/time.c             -o build/lib/time.o             $cc_flags
#i686-elf-gcc -c src/mc/f3f5.c              -o build/mc/f3f5.o              $cc_flags
#i686-elf-gcc -c src/mc/mcprotocol.c        -o build/mc/mcprotocol.o        $cc_flags
i686-elf-gcc -c src/misc/psf_font.c        -o build/misc/psf_font.o        $cc_flags
i686-elf-gcc -c src/net/addr.c             -o build/net/addr.o             $cc_flags
i686-elf-gcc -c src/net/arp.c              -o build/net/arp.o              $cc_flags
i686-elf-gcc -c src/net/buf.c              -o build/net/buf.o              $cc_flags
i686-elf-gcc -c src/net/checksum.c         -o build/net/checksum.o         $cc_flags
i686-elf-gcc -c src/net/dhcp.c             -o build/net/dhcp.o             $cc_flags
i686-elf-gcc -c src/net/dns.c              -o build/net/dns.o              $cc_flags
i686-elf-gcc -c src/net/eth.c              -o build/net/eth.o              $cc_flags
i686-elf-gcc -c src/net/icmp.c             -o build/net/icmp.o             $cc_flags
i686-elf-gcc -c src/net/intf.c             -o build/net/intf.o             $cc_flags
i686-elf-gcc -c src/net/ipv4.c             -o build/net/ipv4.o             $cc_flags
i686-elf-gcc -c src/net/loopback.c         -o build/net/loopback.o         $cc_flags
i686-elf-gcc -c src/net/net.c              -o build/net/net.o              $cc_flags
i686-elf-gcc -c src/net/ntp.c              -o build/net/ntp.o              $cc_flags
i686-elf-gcc -c src/net/port.c             -o build/net/port.o             $cc_flags
i686-elf-gcc -c src/net/route.c            -o build/net/route.o            $cc_flags
i686-elf-gcc -c src/net/tcp.c              -o build/net/tcp.o              $cc_flags
i686-elf-gcc -c src/net/udp.c              -o build/net/udp.o              $cc_flags
i686-elf-gcc -c src/sys/heap.c             -o build/sys/heap.o             $cc_flags -O0
i686-elf-gcc -c src/sys/isrs.c             -o build/sys/isrs.o             $cc_flags -mgeneral-regs-only -Wno-unused-parameter
i686-elf-gcc -c src/sys/kernel_mem.c       -o build/sys/kernel_mem.o       $cc_flags -O0
i686-elf-gcc -c src/sys/panic.c            -o build/sys/panic.o            $cc_flags
i686-elf-gcc -c src/sys/pit.c              -o build/sys/pit.o              $cc_flags
i686-elf-gcc -c src/sys/rtc.c              -o build/sys/rtc.o              $cc_flags
i686-elf-gcc -c src/sys/syscall.c          -o build/sys/syscall.o          $cc_flags -mgeneral-regs-only
i686-elf-gcc -c src/video/graphics.c       -o build/video/graphics.o       $cc_flags
i686-elf-gcc -c src/video/lfb.c            -o build/video/lfb.o            $cc_flags
i686-elf-gcc -c src/video/lfb_terminal.c   -o build/video/lfb_terminal.o   $cc_flags
i686-elf-gcc -c src/video/mouse_renderer.c -o build/video/mouse_renderer.o $cc_flags
i686-elf-gcc -c src/video/vga_terminal.c   -o build/video/vga_terminal.o   $cc_flags
i686-elf-gcc -c mishavfs/vfs.c             -o build/vfs/vfs.o              $cc_flags

objects="
                build/sys/kernel_mem.o \
                build/cpu/idt_s.o \
                build/cpu/idt.o \
                build/cpu/gdt_s.o \
                build/cpu/gdt.o \
                build/cpu/io.o \
                build/cpu/pic.o \
                build/cpu/acpi.o \
                build/cpu/paging.o \
                build/boot.o \
                build/kernel.o \
                build/sys/isrs.o \
                build/sys/panic.o \
                build/sys/rtc.o \
                build/sys/pit.o \
                build/sys/heap.o \
                build/sys/syscall.o \
                build/lib/terminal.o \
                build/lib/string.o \
                build/lib/stdlib.o \
                build/lib/tga.o \
                build/lib/sleep.o \
                build/lib/ctype.o \
                build/lib/time.o \
                build/lib/kprintf.o \
                build/misc/psf_font.o \
                build/dev/pci.o \
                build/dev/driver.o \
                build/dev/storage/ide.o \
                build/dev/net/intel.o \
                build/dev/net/rtl8139.o \
                build/dev/input/mouse.o \
                build/net/addr.o \
                build/net/buf.o \
                build/net/eth.o \
                build/net/intf.o \
                build/net/ipv4.o \
                build/net/checksum.o \
                build/net/route.o \
                build/net/arp.o \
                build/net/net.o \
                build/net/loopback.o \
                build/net/dns.o \
                build/net/udp.o \
                build/net/port.o \
                build/net/dhcp.o \
                build/net/ntp.o \
                build/net/tcp.o \
                build/net/icmp.o \
                build/video/mouse_renderer.o \
                build/video/graphics.o
                build/video/lfb_terminal.o \
                build/video/vga_terminal.o \
                build/video/lfb.o \
                build/vfs/vfs.o \
                "

i686-elf-gcc -T linker.ld -o build/mishaos.bin -ffreestanding -O2 -nostdlib $objects -lgcc

i686-elf-objcopy --only-keep-debug build/mishaos.bin build/mishaos.debug
i686-elf-strip --strip-debug --strip-unneeded build/mishaos.bin

python3 export_debug.py

# Build mishavfs
pushd mishavfs
./build.sh
popd

# Create initrd
rm -f build/initrd.img
mishavfs/build/misha.mkvfs --label=INITRD initrd build/initrd.img

pushd mishaboot/lgbt
./build.sh
popd

pushd mishaboot/eboot
./build.sh
popd

# Create bootable image
mkdir -p build/iso/boot/grub
cp -f grub.cfg build/iso/boot/grub
cp -f build/mishaos.bin build/iso/boot
cp -f build/initrd.img build/iso/boot
grub-mkrescue -o mishaos_grub.iso build/iso