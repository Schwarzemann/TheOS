; enter_usermode(entry, user_stack): iret-drops from ring0 to ring3 at `entry`.
; Never returns: the task re-enters ring0 only via interrupt or syscall.
BITS 32

global enter_usermode

%define GDT_UCODE (0x18 | 3)
%define GDT_UDATA (0x20 | 3)

section .text

enter_usermode:
    mov eax, [esp + 4]      ; entry
    mov ecx, [esp + 8]      ; user stack

    mov bx, GDT_UDATA
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    push dword GDT_UDATA        ; SS
    push ecx                    ; ESP
    pushfd
    pop edx
    or edx, 0x200               ; make sure IF is set
    push edx                    ; EFLAGS
    push dword GDT_UCODE        ; CS
    push eax                    ; EIP
    iret
