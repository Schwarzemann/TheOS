ENTRY(_start)

/* Code from kernel/user/ runs in ring3. It gets its own page-aligned sections,
 * and paging_init() makes only those pages (between the user_* symbols)
 * user-accessible. */

SECTIONS
{
    /* Must match KERNEL_ADDR in boot/stage2.asm */
    . = 0x100000;
    kernel_start = .;

    .text : {
        *(.text.entry)
        *(EXCLUDE_FILE(*/user/*.o) .text)
        *(EXCLUDE_FILE(*/user/*.o) .text.*)
    }

    .rodata : {
        *(EXCLUDE_FILE(*/user/*.o) .rodata)
        *(EXCLUDE_FILE(*/user/*.o) .rodata.*)
    }

    .data : {
        *(EXCLUDE_FILE(*/user/*.o) .data)
        *(EXCLUDE_FILE(*/user/*.o) .data.*)
    }

    /* Ring3 code and constants from kernel/user/: user-readable, read-only. */
    .user_text ALIGN(4096) : {
        user_text_start = .;
        */user/*.o(.text .text.* .rodata .rodata.*)
        . = ALIGN(4096);
        user_text_end = .;
    }

    /* Ring3 globals from kernel/user/: user-writable. */
    .user_data : {
        user_data_start = .;
        */user/*.o(.data .data.* .bss .bss.* COMMON)
        . = ALIGN(4096);
        user_data_end = .;
    }

    bss_start = .;
    .bss : {
        *(COMMON)
        *(.bss)
    }
    bss_end = .;

    kernel_end = .;
}
