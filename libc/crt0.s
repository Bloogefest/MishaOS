.global _start
.extern main
_start:
    pop %eax
    call main
    mov %eax, %ebx
    mov $0, %eax      # sys_exit
    int $0x80
.1: jmp .1
