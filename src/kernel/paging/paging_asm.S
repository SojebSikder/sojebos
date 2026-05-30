.intel_syntax noprefix

.global load_page_directory
load_page_directory:
    mov edx, [esp + 4]
    mov cr3, edx    # load page directory phyisical base address into CR3
    ret

.global enable_paging
enable_paging:
    mov eax, cr0
    or eax, 0x80000000    # set PG bit in CR0
    mov cr0, eax
    ret
