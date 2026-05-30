#include "tss.h"
#include "gdt.h"

#include "../libc/string.h"

static tss_entry_t tss_entry;

void tss_init(uint32_t idx, uint32_t kss, uint32_t kesp) {
  // Clear the TSS memory
  memset(&tss_entry, 0, sizeof(tss_entry_t));

  // Set the kernel stack segments
  tss_entry.ss0 = kss;   // Usually 0x10 (Kernel Data Segment)
  tss_entry.esp0 = kesp; // The top of the kernel stack

  // Set the I/O Map Base to the size of the TSS to disable the I/O permission bitmap
  tss_entry.iomap_base = sizeof(tss_entry_t);

  // Calculate base and limit for the GDT entry
  uint32_t base = (uint32_t)&tss_entry;
  uint32_t limit = sizeof(tss_entry_t) - 1;

  /**
   * GDT Entry for TSS:
   * Access: 0x89 (Present, Executable, Accessible from Ring 0)
   * Limit: sizeof(tss) - 1
   */
  gdt_set_gate(idx, base, limit, 0x89, 0x00);

  // Load the TSS into the CPU
  // The selector is (idx * 8). If idx is 5, selector is 0x28.
  uint16_t selector = (idx * 8);
  asm volatile("ltr %0" : : "r"(selector));
}

void tss_set_stack(uint32_t kss, uint32_t kesp) {
  tss_entry.ss0 = kss;
  tss_entry.esp0 = kesp;
}
