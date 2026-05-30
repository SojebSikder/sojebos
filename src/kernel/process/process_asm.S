.intel_syntax noprefix

.global enter_user_mode

enter_user_mode:
    mov edx, [esp + 4]    # Argument 1: Target entry point address
    mov ecx, [esp + 8]    # Argument 2: Target user stack pointer

    # Set up Data segment registers with the User Data Descriptor index
    # assuming GDT layout: Ring 3 Data is usually offset 0x23 (Index 4 | RPL 3)
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    # Build the custom stack frame required by IRET
    push 0x23             # SS (User Data Segment selector with RPL 3)
    push ecx              # ESP (User Stack address pointer)
    pushf                 # EFLAGS

    # Pop EFLAGS into EAX, turn on Interrupt Flag (0x200), push back
    pop eax
    or eax, 0x200
    push eax

    push 0x1B             # CS (User Code Segment selector with RPL 3, e.g., 0x18 | 3)
    push edx              # EIP (Target entry point instruction pointer)

    # Execute IRET. The CPU pops these fields, shifts to Ring 3, and jumps to entry point.
    iret
