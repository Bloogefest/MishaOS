.intel_syntax noprefix

gdtr: .short 0
      .int 0

.global gdt_load
gdt_load:
    mov ax, [esp + 4]
    mov [gdtr], ax
    mov eax, [esp + 8]
    mov [gdtr + 2], eax
    lgdt [gdtr]
    ret
