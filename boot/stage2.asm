; TheBOOT stage 2: loads the kernel, enables A20, detects RAM and disk, enters protected mode.
BITS 16
ORG 0x7E00

KERNEL_BUFF_SEG   equ 0x1000          ; -> physical 0x10000
KERNEL_BUFF_OFF   equ 0x0000
KERNEL_BUFF_PHYS  equ 0x10000
KERNEL_LBA        equ 5               ; right after stage 2 (1 + 4)
KERNEL_SECTORS    equ 256             ; 128 KiB reserved for the kernel; keep in sync with scripts/layout.sh
KERNEL_BYTES      equ KERNEL_SECTORS * 512

; Each INT 13h read is at most KERNEL_CHUNK sectors, so its transfer stays
; inside one 64 KiB real-mode segment.
KERNEL_CHUNK      equ 64
KERNEL_ADDR       equ 0x100000        ; 1 MiB, where the kernel is linked

CODE_SEG equ 0x08                     ; GDT selector for the code segment
DATA_SEG equ 0x10                     ; GDT selector for the data segment

BOOT_MEM_KB  equ 0x0500
BOOT_DISK_KB equ 0x0504

start:
    mov [boot_drive], dl

    mov si, msg_stage2
    call print_string

    mov si, msg_load_kernel
    call print_string

    ; BIOS reads go to the real-mode buffer at 0x1000:0x0000; the image is
    ; copied to KERNEL_ADDR after entering protected mode.
    mov word [sectors_left], KERNEL_SECTORS
    mov dword [cur_lba], KERNEL_LBA
    mov word [cur_seg], KERNEL_BUFF_SEG

.load_chunk:
    mov ax, [sectors_left]
    cmp ax, 0
    je .load_done
    cmp ax, KERNEL_CHUNK
    jbe .chunk_ok
    mov ax, KERNEL_CHUNK
.chunk_ok:
    mov [dap.count], ax

    mov word [dap.offset], KERNEL_BUFF_OFF
    mov bx, [cur_seg]
    mov [dap.segment], bx

    mov eax, [cur_lba]
    mov [dap.lba_low], eax
    mov dword [dap.lba_high], 0

    mov dl, [boot_drive]
    mov si, dap
    mov ah, 0x42
    int 0x13
    jc disk_error

    movzx eax, word [dap.count]
    add [cur_lba], eax
    sub [sectors_left], ax

    ; advance the destination segment by (sectors * 512) / 16 bytes
    mov ax, [dap.count]
    shl ax, 5
    add [cur_seg], ax

    jmp .load_chunk
.load_done:

    mov si, msg_kloaded
    call print_string

    call enable_a20
    mov si, msg_a20
    call print_string

    call detect_memory
    mov si, msg_ram_prefix
    call print_string
    mov eax, [BOOT_MEM_KB]
    call print_dec32
    mov si, msg_kb_suffix
    call print_string

    call detect_disk
    mov si, msg_disk_pfx
    call print_string
    mov eax, [BOOT_DISK_KB]
    call print_dec32
    mov si, msg_kb_suffix
    call print_string

    mov si, msg_reloc_pfx
    call print_string
    mov eax, KERNEL_BUFF_PHYS
    call print_hex32
    mov si, msg_arrow
    call print_string
    mov eax, KERNEL_ADDR
    call print_hex32
    mov si, msg_size_prefix
    call print_string
    mov eax, KERNEL_BYTES
    call print_hex32
    mov si, msg_bytes
    call print_string

    mov si, msg_pmode
    call print_string

    cli
    lgdt [gdt_desc]

    mov eax, cr0
    or eax, 1                  ; set PE bit
    mov cr0, eax

    jmp CODE_SEG:pmode_entry

disk_error:
    mov si, msg_error
    call print_string
    jmp $

; print_string: prints a null-terminated string via BIOS teletype
; in: SI = pointer to string

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

; print_hex32: prints EAX as an 8-digit hex physical address
; in: EAX = value to print

print_hex32:
    pushad
    mov ebx, eax
    mov cx, 8
    mov si, hex_buf
.loop:
    rol ebx, 4          ; rotate next nibble (MSB-first) into bl
    mov al, bl
    and al, 0x0F
    cmp al, 10
    jl .digit
    add al, 'A' - 10
    jmp .store
.digit:
    add al, '0'
.store:
    mov [si], al
    inc si
    loop .loop
    mov byte [si], 0
    mov si, hex_buf
    call print_string
    popad
    ret

; print_dec32: prints EAX as unsigned decimal, no leading zeros
; in: EAX = value to print

print_dec32:
    pushad
    xor ecx, ecx            ; digit count
    mov ebx, 10
    test eax, eax
    jnz .divloop
    mov al, '0'
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp .done
.divloop:
    xor edx, edx
    div ebx                 ; eax /= 10, edx = remainder digit
    push dx
    inc ecx
    test eax, eax
    jnz .divloop
.printloop:
    pop dx
    mov al, dl
    add al, '0'
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    loop .printloop
.done:
    popad
    ret

; enable_a20: fast A20 gate enable via port 0x92

enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

; detect_memory: stores RAM in KB at BOOT_MEM_KB. Uses INT 15h AX=E801h, falling
; back to AH=88h (max ~64 MiB). Both skip the first MiB, which is added as 1024 KB.

detect_memory:
    pusha
    mov ax, 0xE801
    int 0x15
    jc .use_ah88
    cmp ah, 0x86             ; unsupported function
    je .use_ah88
    cmp ah, 0x80
    je .use_ah88
    test ax, ax
    jnz .have_e801
    test bx, bx
    jnz .have_e801
    mov ax, cx                ; some BIOSes return the counts in cx:dx instead
    mov bx, dx
.have_e801:
    movzx eax, ax
    movzx ebx, bx
    shl ebx, 6                ; bx * 64 KiB, expressed in KiB
    add eax, ebx
    add eax, 1024
    mov [BOOT_MEM_KB], eax
    popa
    ret
.use_ah88:
    mov ah, 0x88
    int 0x15
    jc .fail
    movzx eax, ax
    add eax, 1024
    mov [BOOT_MEM_KB], eax
    popa
    ret
.fail:
    mov dword [BOOT_MEM_KB], 0
    popa
    ret

; detect_disk: stores the boot drive size in KB at BOOT_DISK_KB, from INT 13h AH=48h.
; Assumes 512-byte sectors, so KB = sectors / 2.

detect_disk:
    pusha
    mov word [disk_buf], disk_buf_end - disk_buf
    mov ah, 0x48
    mov dl, [boot_drive]
    mov si, disk_buf
    int 0x13
    jc .fail
    mov eax, [disk_buf + 0x10]    ; low dword of total sector count
    shr eax, 1                         ; sectors * 512 bytes / 1024 = sectors / 2
    mov [BOOT_DISK_KB], eax
    popa
    ret
.fail:
    mov dword [BOOT_DISK_KB], 0
    popa
    ret

boot_drive: db 0
sectors_left: dw 0
cur_lba:            dd 0
cur_seg:        dw 0
msg_stage2:          db "TheBOOT: Stage2 running at phys 0x00007E00", 13, 10, 0
msg_load_kernel:      db "TheBOOT: loading kernel  LBA=5 sectors=256 -> phys 0x00010000", 13, 10, 0
msg_kloaded:    db "TheBOOT: kernel loaded OK", 13, 10, 0
msg_a20:              db "TheBOOT: A20 line enabled", 13, 10, 0
msg_reloc_pfx:  db "TheBOOT: relocating kernel  phys 0x", 0
msg_arrow:            db " -> phys 0x", 0
msg_size_prefix:      db "  size=0x", 0
msg_bytes:            db " bytes", 13, 10, 0
msg_pmode:            db "TheBOOT: entering 32-bit protected mode...", 13, 10, 0
msg_ram_prefix:       db "TheBOOT: RAM detected: ", 0
msg_disk_pfx:      db "TheBOOT: Disk size: ", 0
msg_kb_suffix:        db " KB", 13, 10, 0
msg_error:            db "TheBOOT: disk read error!", 13, 10, 0

hex_buf: times 9 db 0

align 4
dap:
    .size     db 0x10
    .reserved db 0
    .count    dw 0
    .offset   dw 0
    .segment  dw 0
    .lba_low  dd 0
    .lba_high dd 0

; INT 13h AH=48h needs the buffer size in its first word. 30 bytes covers
; the bytes-per-sector field.
align 4
disk_buf: times 30 db 0
disk_buf_end:

; Flat 32-bit GDT: null, code (0-4GiB), data (0-4GiB)

align 8
gdt_start:
gdt_null:
    dq 0
gdt_code:
    dw 0xFFFF       ; limit 0-15
    dw 0x0000       ; base 0-15
    db 0x00         ; base 16-23
    db 10011010b    ; access: present, ring0, code, executable, readable
    db 11001111b    ; flags (4KiB gran, 32-bit) + limit 16-19
    db 0x00         ; base 24-31
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b    ; access: present, ring0, data, writable
    db 11001111b
    db 0x00
gdt_end:

gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; 32-bit protected mode code

BITS 32
pmode_entry:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; Relocate kernel to its linked address
    mov esi, KERNEL_BUFF_PHYS
    mov edi, KERNEL_ADDR
    mov ecx, KERNEL_BYTES
    cld
    rep movsb

    jmp KERNEL_ADDR
