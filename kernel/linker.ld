ENTRY(_start)

SECTIONS
{
    /* Must match KERNEL_ADDR in boot/stage2.asm */
    . = 0x100000;
    kernel_start = .;

    .text : {
        *(.text.entry)
        *(.text)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data)
    }

    bss_start = .;
    .bss : {
        *(COMMON)
        *(.bss)
    }
    bss_end = .;

    kernel_end = .;
}
