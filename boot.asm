; ==========================================
; SojebOS Bootloader - 16-bit to 32-bit PM
; ==========================================

BITS 16
ORG 0x7C00

start:
    mov [BOOT_DRIVE], dl        ; Save boot drive provided by BIOS

    ; Setup stack and segments for Real Mode
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Print boot message
    mov si, boot_msg
.print:
    lodsb
    or al, al
    jz load_kernel
    mov ah, 0x0E
    int 0x10
    jmp .print

; Load Kernel (15 sectors to be safe, starting from sector 2)
load_kernel:
    ; Reset disk system (Drive 0 is often unstable without a reset)
    mov ah, 0
    int 0x13

    mov bx, 0x1000
    mov ah, 0x02
    mov al, 15
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error

    ; --- Switch to 32-bit Protected Mode ---
    cli                     ; 1. Disable interrupts
    lgdt [gdt_descriptor]   ; 2. Load the Global Descriptor Table
    mov eax, cr0
    or eax, 0x1             ; 3. Set the first bit of CR0 (Control Register 0)
    mov cr0, eax
    jmp CODE_SEG:init_pm    ; 4. Far jump to flush the CPU pipeline

; ------------------------------------------
; Error Handling
; ------------------------------------------

disk_error:
    mov si, disk_msg
.err_print:
    lodsb
    or al, al
    jz $                    ; Hang if error occurs
    mov ah, 0x0E
    int 0x10
    jmp .err_print

; ------------------------------------------
; 32-bit Protected Mode Entry
; ------------------------------------------

[BITS 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Move stack further up to avoid overwriting kernel code
    mov ebp, 0x90000
    mov esp, ebp

    ; Add a small 'health check' - write a character directly to video memory
    ; This proves we are successfully in 32-bit mode
    mov byte [0xB8000], 'P'
    mov byte [0xB8001], 0x0F

    call 0x1000             ; Jump to kernel
    jmp $

; ------------------------------------------
; GDT (Global Descriptor Table)
; ------------------------------------------

gdt_start:
    dq 0x0                  ; Null descriptor (required)

gdt_code:                   ; Code segment descriptor
    dw 0xffff               ; Limit (0-15 bits)
    dw 0x0                  ; Base (0-15 bits)
    db 0x0                  ; Base (16-23 bits)
    db 10011010b            ; Access byte
    db 11001111b            ; Flags + Limit (16-19 bits)
    db 0x0                  ; Base (24-31 bits)

gdt_data:                   ; Data segment descriptor
    dw 0xffff               ; Limit
    dw 0x0                  ; Base
    db 0x0                  ; Base
    db 10010010b            ; Access byte
    db 11001111b            ; Flags
    db 0x0                  ; Base

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; Size of GDT
    dd gdt_start               ; Address of GDT

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; ------------------------------------------
; Data & Signature
; ------------------------------------------

boot_msg   db "Booting SojebOS into 32-bit...", 13, 10, 0
disk_msg   db "Disk Read Error!", 0
BOOT_DRIVE db 0

times 510 - ($ - $$) db 0
dw 0xAA55
