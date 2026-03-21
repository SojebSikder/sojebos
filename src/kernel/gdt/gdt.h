#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// Each GDT entry is 8 bytes
struct gdt_entry_struct {
    uint16_t limit_low;    // The lower 16 bits of the limit
    uint16_t base_low;     // The lower 16 bits of the base
    uint8_t  base_middle;  // The next 8 bits of the base
    uint8_t  access;       // Access flags
    uint8_t  granularity;
    uint8_t  base_high;    // The last 8 bits of the base
} __attribute__((packed));

typedef struct gdt_entry_struct gdt_entry_t;

// Pointer structure for the 'lgdt' instruction
struct gdt_ptr_struct {
    uint16_t limit;        // The size of the table minus 1
    uint32_t base;         // The address of the first gdt_entry_t
} __attribute__((packed));

typedef struct gdt_ptr_struct gdt_ptr_t;

// Standard GDT functions
void gdt_init();
void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

#endif
