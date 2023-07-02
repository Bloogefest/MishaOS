.section .text

.code64
.global call_kernel64
call_kernel64:
    cli
    mov %eax, 0x7C00
    mov %ebx, 0x7C04
    mov %ecx, 0x7C08
    mov %cr0, %rax
    and $0x7FFEFFFF, %eax
    mov %rax, %cr0
    mov $0xC0000080, %ecx
    rdmsr
    and $0xFFFFFEFF, %eax
    wrmsr
    mov $0x640, %rax
    mov %rax, %cr4
    mov 0x7C00, %eax
    mov 0x7C04, %ebx
    mov 0x7C08, %ecx

.code32
.global call_kernel32
call_kernel32:
    jmp *%ecx

loop:
    hlt
    jmp loop