; TheBOOT stage 1: loads stage 2 from disk and jumps to it.
BITS 16
ORG 0x7C00

STAGE2_SEG      equ 0x0000
STAGE2_OFFSET   equ 0x7E00
STAGE2_LBA      equ 1               ; sector right after the boot sector
STAGE2_SECTORS  equ 4               ; 2 KiB reserved for stage 2

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl       ; BIOS passes boot drive number in dl

    call blue_screen

    mov si, msg_stage1
    call print_string

    mov si, msg_load_stage2
    call print_string

    ; Fill in the Disk Address Packet and read stage 2 via LBA
    mov word [dap.count],   STAGE2_SECTORS
    mov word [dap.offset],  STAGE2_OFFSET
    mov word [dap.segment], STAGE2_SEG
    mov dword [dap.lba_low], STAGE2_LBA
    mov dword [dap.lba_high], 0

    mov dl, [boot_drive]
    mov si, dap
    mov ah, 0x42                ; INT 13h, AH=42h: extended read
    int 0x13
    jc disk_error

    mov si, msg_stage2_ok
    call print_string

    mov dl, [boot_drive]        ; hand the boot drive on to stage 2
    jmp STAGE2_SEG:STAGE2_OFFSET

disk_error:
    mov si, msg_error
    call print_string
    jmp $

blue_screen:
    push es
    push ax
    push cx
    push di
    mov ax, 0xB800
    mov es, ax
    xor di, di
    mov cx, 80*25
    mov ax, 0x1F20      ; attribute 0x1F, char ' '
.fill:
    stosw
    loop .fill
    pop di
    pop cx
    pop ax
    pop es
    ret

; print_string: prints a null-terminated string via BIOS teletype. In: SI = string
print_string:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp .loop
.done:
    popa
    ret

boot_drive: db 0
msg_stage1:      db "TheBOOT: Stage1 running at phys 0x00007C00", 13, 10, 0
msg_load_stage2: db "TheBOOT: loading Stage2  LBA=1 sectors=4 -> phys 0x00007E00", 13, 10, 0
msg_stage2_ok:   db "TheBOOT: Stage2 loaded OK, jumping to phys 0x00007E00", 13, 10, 0
msg_error:       db "TheBOOT: disk read error!", 13, 10, 0

align 4
dap:
    .size     db 0x10
    .reserved db 0
    .count    dw 0
    .offset   dw 0
    .segment  dw 0
    .lba_low  dd 0
    .lba_high dd 0

times 510 - ($ - $$) db 0
dw 0xAA55
