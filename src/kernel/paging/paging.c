#include "paging.h"

// aligning to 4096 bytes (Page boundary) is strict
__attribute__((aligned(4096))) uint32_t page_directory[1024];

// we will preallocate 4 page tables to comfortably cover 16MB of memory
__attribute__((aligned(4096))) uint32_t first_page_tables[4][1024];

// physical frame allocator
#define MAX_PHYSICAL_MEMORY (64 * 1024 * 1024) // 64MB
#define PAGE_SIZE 4096
#define TOTAL_FRAMES (MAX_PHYSICAL_MEMORY / PAGE_SIZE)
#define BITMAP_SIZE (TOTAL_FRAMES / 32)

static uint32_t frame_bitmap[BITMAP_SIZE];

static void bitmap_set(uint32_t frame_idx) {
  frame_bitmap[frame_idx / 32] |= (1 << (frame_idx % 32));
}

static void bitmap_clear(uint32_t frame_idx) {
  frame_bitmap[frame_idx / 32] &= !(1 << (frame_idx % 32));
}

// test if a bit is set
static int bitmap_test(uint32_t frame_idx) {
  return (frame_bitmap[frame_idx / 32] & (1 << (frame_idx % 32)));
}

// allocate a physical frame and return its phyisical address
void *alloc_frame() {
  for (uint32_t i = 0; i < TOTAL_FRAMES; i++) {
    if (!bitmap_test(i)) {
      bitmap_set(i);
      return (void *)(i * PAGE_SIZE);
    }
  }
  return NULL;
}

// frees a physical frame
void free_frame(void *physaddr) {
  uint32_t frame_idx = ((uint32_t)physaddr) / PAGE_SIZE;
  bitmap_clear(frame_idx);
}

extern void load_page_directory(uint32_t *);
extern void enable_paging(void);

void paging_init(uint32_t kernel_end_paddr) {
  // clear page directory
  for (int i = 0; i < 1024; i++) {
    // set attributes: Supervisor, Writeable, Not Present
    page_directory[i] = 0x00000002;
  }

  // initialize the physical frame allocation bitmap
  // clear all frames to 0
  for (int i = 0; i < BITMAP_SIZE; i++) {
    frame_bitmap[i] = 0;
  }

  // mark memory used by kernel + heap as "Reserved" in our bitmap
  // Align kernel_end_paddr upword to next page boundary
  uint32_t reserved_end_page = (kernel_end_paddr + PAGE_SIZE - 1) / PAGE_SIZE;
  for (uint32_t i = 0; i < reserved_end_page; i++) {
    bitmap_set(i);
  }

  // identity map the first 16MB of RAM (covers kernel, heap, MMIO, BSS)
  uint32_t address = 0;
  for (int table_idx = 0; table_idx < 4; table_idx++) {
    for (int entry_idx = 0; entry_idx < 1024; entry_idx++) {
      // map the physical address with attributes: Present, Read/Write
      // 0x3 = Present | Read/Write
      first_page_tables[table_idx][entry_idx] =
          address | (PAGE_PRESENT | PAGE_RW);
      address += PAGE_SIZE; // move to next page frame
    }

    // put the page table into the page directory
    // 0x3 = Present | Read/Write
    page_directory[table_idx] =
        ((uint32_t)first_page_tables[table_idx]) | (PAGE_PRESENT | PAGE_RW);
  }

  // register page directory with the CPU and toggle the CR0 register bit
  load_page_directory(page_directory);
  enable_paging();
}

void map_page(void *physaddr, void *virtualaddr, uint32_t flags) {
  uint32_t vaddr = (uint32_t)virtualaddr;
  uint32_t paddr = (uint32_t)physaddr;

  // extract directory and table indices
  uint32_t pd_idx = vaddr >> 22;
  uint32_t pt_idx = (vaddr >> 12) & 0x3FF;

  uint32_t *page_table;

  // check if the page table exists in the directory
  if (page_directory[pd_idx] & PAGE_PRESENT) {
    // clear lower 12 bits attribute flags to get the pure phyisical pointer
    page_table = (uint32_t *)(page_directory[pd_idx] & ~0xFFF);
  } else {
    // dynamically allocate a new page table frame
    void *new_table_frame = alloc_frame();
    if (!new_table_frame) {
      // out of memory mapping page tables
      return;
    }

    page_table = (uint32_t *)new_table_frame;
    // clear out the new created page table entries
    for (int i = 0; i < 1024; i++) {
      page_table[i] = 0;
    }

    // map the new page table into our directory
    page_directory[pd_idx] =
        ((uint32_t)page_table) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
  }

  // map the physical page address to the page table with requested flags
  page_table[pt_idx] = (paddr & ~0xFFF) | (flags & 0xFFF);

  // INVLPG: invalidate the page cache for the virtual address
  // Whenever modify an existing page active page table mapping,
  // we must invalidate the CPU's TLB cache for that specific address
  asm volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
}
