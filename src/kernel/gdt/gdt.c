#include "gdt.h"

// We define 6 entries: Null, K-Code, K-Data, U-Code, U-Data, and TSS
static gdt_entry_t gdt_entries[6];
static gdt_ptr_t gdt_ptr;

// This assembly function is usually defined in your boot/entry file
extern void gdt_flush(uint32_t);

void gdt_init() {
  gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
  gdt_ptr.base = (uint32_t)&gdt_entries;

  // 0x00: Null segment (Required)
  gdt_set_gate(0, 0, 0, 0, 0);

  // 0x08: Kernel Code Segment
  // Access: 0x9A (Present, Ring 0, Exec/Read)
  // Gran: 0xCF (4KB granularity, 32-bit mode)
  gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

  // 0x10: Kernel Data Segment
  // Access: 0x92 (Present, Ring 0, Read/Write)
  gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

  // 0x18: User Code Segment (Ring 3)
  // Access: 0xFA (Present, Ring 3, Exec/Read)
  gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

  // 0x20: User Data Segment (Ring 3)
  // Access: 0xF2 (Present, Ring 3, Read/Write)
  gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

  // Entry 5 is reserved for TSS and will be set by tss_init()

  // Inform the CPU about our new table
  gdt_flush((uint32_t)&gdt_ptr);
}

void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access,
                  uint8_t gran) {
  gdt_entries[num].base_low = (base & 0xFFFF);
  gdt_entries[num].base_middle = (base >> 16) & 0xFF;
  gdt_entries[num].base_high = (base >> 24) & 0xFF;

  gdt_entries[num].limit_low = (limit & 0xFFFF);
  gdt_entries[num].granularity = (limit >> 16) & 0x0F;

  gdt_entries[num].granularity |= gran & 0xF0;
  gdt_entries[num].access = access;
}
