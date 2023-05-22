.set ALIGN, 1<<0
.set MEMINFO, 1<<1
.set FLAGS, ALIGN | MEMINFO
.set MAGIC, 0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
multiboot:
    .long MAGIC
    .long FLAGS
    .long CHECKSUM
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0
    .long 0
    .long 32

.section .bss
.align 16
.skip 16384

stack_top:

.section .text
.global _start
.type _start, @function
_start:
    mov $stack_top, %esp
    push %esp
    push %eax
    push %ebx
    call kernel_main
    cli
1:  hlt
    jmp 1b

.size _start, . - _start
