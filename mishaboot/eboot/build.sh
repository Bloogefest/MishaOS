#!/bin/bash
# TODO: Makefile
set -e

rm -rf build
rm -f uefi.iso uefi.cdr
mkdir -p build

export CFLAGS="-I $(pwd)"
pushd gnu-efi
export CROSS_COMPILE="i686-elf-"
ARCH=ia32 make
popd

pushd gnu-efi
export CROSS_COMPILE="x86_64-elf-"
ARCH=x86_64 make
popd

flags="-Ignu-efi/inc -I. -Wno-error=pragmas -mno-red-zone -mno-avx -fPIE -g -O0 -Wall -Wextra -Wno-pointer-sign -Werror -funsigned-char -fshort-wchar -fno-strict-aliasing -ffreestanding -fno-stack-protector -fno-stack-check -fno-merge-all-constants -Wno-error=unused-parameter -Wno-error=unused-variable -std=gnu99 -DGNU_EFI_USE_MS_ABI -maccumulate-outgoing-args -D__KERNEL__"

i686-elf-gcc $flags -c bootloader.c -o build/bootloader.o
i686-elf-ld -nostdlib --warn-common --no-undefined --fatal-warnings --build-id=sha1 -z nocombreloc -shared -Bsymbolic -Lgnu-efi/ia32/lib -Lgnu-efi/ia32/gnuefi -Tgnu-efi/gnuefi/elf_ia32_efi.lds gnu-efi/ia32/gnuefi/crt0-efi-ia32.o build/bootloader.o -o build/bootloader.so -lgnuefi -lefi $(i686-elf-gcc -print-libgcc-file-name)
i686-elf-objcopy -j .text -j .sdata -j .data -j .dynamic -j .rodata -j .rel -j .rela -j .rel.* -j .rela.* -j .rel* -j .rela* -j .areloc -j .reloc --target efi-app-ia32 build/bootloader.so build/BOOTIA32.EFI

x86_64-elf-gcc $flags -c bootloader.c -o build/bootloader.o
x86_64-elf-ld -nostdlib --warn-common --no-undefined --fatal-warnings --build-id=sha1 -z nocombreloc -shared -Bsymbolic -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -Tgnu-efi/gnuefi/elf_x86_64_efi.lds gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o build/bootloader.o -o build/bootloader.so -lgnuefi -lefi $(x86_64-elf-gcc -print-libgcc-file-name)
x86_64-elf-objcopy -j .text -j .sdata -j .data -j .dynamic -j .rodata -j .rel -j .rela -j .rel.* -j .rela.* -j .rel* -j .rela* -j .areloc -j .reloc --target efi-app-x86_64 build/bootloader.so build/BOOTX64.EFI

mkdir -p build/contents
cp -f ../../build/mishaos.bin build/KERNEL.ELF
cp -f ../../build/initrd.img build/INITRD.IMG

dd if=/dev/zero of=build/contents/fat.img bs=1k count=1440
mformat -i build/contents/fat.img -f 1440 ::
mmd -i build/contents/fat.img ::/EFI
mmd -i build/contents/fat.img ::/EFI/BOOT
mcopy -i build/contents/fat.img startup.nsh ::
mcopy -i build/contents/fat.img build/BOOTIA32.EFI ::/EFI/BOOT
mcopy -i build/contents/fat.img build/BOOTX64.EFI ::/EFI/BOOT
mcopy -i build/contents/fat.img build/KERNEL.ELF ::
mcopy -i build/contents/fat.img build/INITRD.IMG ::
cp -f build/contents/fat.img ../../mishaos_eboot.img
xorriso -as mkisofs -R -J -c bootcat -eltorito-alt-boot -e fat.img -no-emul-boot -isohybrid-gpt-basdat -o ../../mishaos_eboot.iso build/contents