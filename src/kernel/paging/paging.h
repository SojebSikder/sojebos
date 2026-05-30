#ifndef PAGING_H
#define PAGING_H

#include <stddef.h>
#include <stdint.h>

#define PAGE_PRESENT 0x1
#define PAGE_RW 0x2
#define PAGE_USER 0x4

// initialize paging and identity mapping the kernel space
void paging_init(uint32_t kernel_end_paddr);

// helper to map a virtual page to a phyisical frame
void map_page(void *physaddr, void *virtualaddr, uint32_t flags);

// phyisical frame allocation
void *alloc_frame();
void free_frame(void *physaddr);

#endif
