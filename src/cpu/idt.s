.intel_syntax noprefix
.section .text

idtr: .short 0
      .int 0

.global idt_load
idt_load:
    mov ax, [esp + 4]
    mov [idtr], ax
    mov eax, [esp + 8]
    mov [idtr + 2], eax
    lidt [idtr]
    ret
