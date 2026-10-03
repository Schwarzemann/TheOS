; void task_switch(uint32_t *old_esp_store, uint32_t new_esp): saves callee-saved regs and EFLAGS,
; stores ESP, then restores the new task's frame (task.c primes new stacks to match).
BITS 32

global task_switch

section .text

task_switch:
    push ebp
    push ebx
    push esi
    push edi
    pushfd

    mov eax, [esp + 24]     ; old_esp_store
    mov [eax], esp

    mov esp, [esp + 28]     ; new_esp

    popfd
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
