ENTRY(_start)

SECTIONS
{
    /* The kernel usually lives at 1MB */
    . = 1M;

    /* First and most important: the Multiboot header */
    .text :
    {
        *(.multiboot)
        *(.text)
    }

    .rodata : { *(.rodata) }
    .data   : { *(.data) }
    .bss    : { *(.bss)  }
}
