; TheOS kernel entry: sets up the stack, zeroes .bss, calls kernel_main().
; Linked at 0x100000 to match KERNEL_ADDR in stage2.asm.
BITS 32

extern kernel_main
extern bss_start
extern bss_end
global _start

; Own section so linker.ld places _start at the kernel's first byte.
section .text.entry
_start:
    mov esp, stack_top

    ; Firmware may leave RAM dirty and kernel.bin doesn't carry .bss.
    mov edi, bss_start
    mov ecx, bss_end
    sub ecx, edi
    xor eax, eax
    cld
    rep stosb

    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384          ; 16 KiB kernel stack
stack_top:
