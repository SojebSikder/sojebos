#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

// Represents a chunk of memory
typedef struct MemoryBlock {
  size_t size;              // Size of the usable data area
  int free;                 // 1 if available, 0 if used
  struct MemoryBlock *next; // Pointer to the next memory block
} MemoryBlock;

#define Block_SIZE sizeof(MemoryBlock)
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

// Initializes the heap with a start address and total size
void kheap_init(void* start, size_t size);

// Allocate memory
void* kmalloc(size_t size);

// Free memory
void kfree(void* ptr);

#endif
