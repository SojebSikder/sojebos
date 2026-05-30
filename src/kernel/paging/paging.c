#include "paging.h"

// aligning to 4096 bytes (Page boundary) is strict
__attribute__((aligned(4096))) uint32_t page_directory[1024];

// we will preallocate 4 page tables to comfortably cover 16MB of memory
__attribute__((aligned(4096))) uint32_t first_page_tables[4][1024];

extern void load_page_directory(uint32_t *);
extern void enable_paging(void);

void paging_init(uint32_t kernel_end_paddr) {
  // clear page directory
  for (int i = 0; i < 1024; i++) {
    // set attributes: Supervisor, Writeable, Not Present
    page_directory[i] = 0x00000002;
  }

  // identity map the first 16MB of RAM (covers kernel, heap, MMIO, BSS)
  uint32_t address = 0;
  for (int table_idx = 0; table_idx < 4; table_idx++) {
    for (int entry_idx = 0; entry_idx < 1024; entry_idx++) {
      // map the physical address with attributes: Present, Read/Write
      // 3 = Present | Read/Write
      first_page_tables[table_idx][entry_idx] = address | 3;
      address += 4096; // move to next page frame
    }

    // put the page table into the page directory
    // 3 = Present | Read/Write
    page_directory[table_idx] = ((uint32_t)first_page_tables[table_idx]) | 3;
  }

  // register page directory with the CPU and toggle the CR0 register bit
  load_page_directory(page_directory);
  enable_paging();
}
