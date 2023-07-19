.intel_syntax noprefix
.section .text

gdtr: .short 0
      .int 0

.global gdt_load
gdt_load:
    mov ax, [esp + 4]
    mov [gdtr], ax
    mov eax, [esp + 8]
    mov [gdtr + 2], eax
    lgdt [gdtr]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
	jmp 0x08:complete_flush
complete_flush:
	ret

.global tss_flush
tss_flush:
	mov ax, 0x2B
	ltr ax
	ret
