# Multiboot constants
.set MAGIC,    0x1BADB002
.set FLAGS,    0x00000003
.set CHECKSUM, -(MAGIC + FLAGS)

# Multiboot header for GRUB
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Reserve stack space (16KB)
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

.section .text
.global _start
.global gdt_flush
.extern kernel_main

_start:
    # 1. Setup our own stack
    mov $stack_top, %esp

    # 2. Call the C kernel entry point
    call kernel_main

    # 3. If kernel returns, halt the CPU
    cli
1:  hlt
    jmp 1b

# --- GDT Flusher ---
# This function is called from gdt.c
# void gdt_flush(uint32_t gdt_ptr_addr)
gdt_flush:
    mov 4(%esp), %eax   # Get the pointer passed from C
    lgdt (%eax)         # Load the new GDT

    # Reload data segments
    mov $0x10, %ax      # 0x10 is our Kernel Data Segment (Offset 16)
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Perform a far jump to reload the CS (Code Segment) register
    # 0x08 is our Kernel Code Segment (Offset 8)
    pushl $0x08
    pushl $.flush
    lret

.flush:
    ret
