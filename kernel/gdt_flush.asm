; gdt_flush(gdt_ptr) loads a new GDT and reloads the segment registers.
; tss_flush() loads the TSS selector.
BITS 32

global gdt_flush
global tss_flush

section .text

gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, 0x10            ; GDT_KDATA
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.flush_cs       ; GDT_KCODE
.flush_cs:
    ret

tss_flush:
    mov ax, 0x28 | 0         ; GDT_TSS, RPL 0
    ltr ax
    ret
