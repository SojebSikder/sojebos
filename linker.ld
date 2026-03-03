ENTRY(kernel_main)

SECTIONS
{
    . = 0x1000;
    .text : {
        *(.text.kernel_main) /* Force kernel_main to be FIRST */
        *(.text*)
    }
    .data : { *(.data*) }
    .bss  : { *(.bss*)  }
}
