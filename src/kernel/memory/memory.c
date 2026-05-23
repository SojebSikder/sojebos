//
// Memory Allocator implemented using a First-Fit Linked List Allocator
//

#include "memory.h"
#include "../libc/string.h"
#include <stdint.h>

static MemoryBlock *free_list_head = NULL;

void kheap_init(void *start, size_t size) {
  free_list_head = (MemoryBlock *)start;
  free_list_head->size = size - Block_SIZE;
  free_list_head->free = 1;
  free_list_head->next = NULL;
}

void *kmalloc(size_t size) {
  size = ALIGN(size);
  MemoryBlock *current = free_list_head;

  // Iterate through the free list to find a suitable block
  while (current) {
    if (current->free && current->size >= size) {
      // Can we split this block?
      // Only split if there's room for a new header + at least 8 bytes of data
      if (current->size >= (size + Block_SIZE + ALIGNMENT)) {
        MemoryBlock *new_block =
            (MemoryBlock *)((uint8_t *)current + Block_SIZE + size);
        new_block->size = current->size - size - Block_SIZE;
        new_block->free = 1;
        new_block->next = current->next;

        current->size = size;
        current->next = new_block;
      }

      current->free = 0;
      // Return pointer to the data area (skipping the header)
      return (void *)(current + 1);
    }
    current = current->next;
  }
  return NULL; // Out of memory
}

void *krealloc(void *ptr, size_t size) {
  // if ptr is NULL, then behave like kmalloc
  if (!ptr) {
    return kmalloc(size);
  }

  // if size is 0 and ptr is not NULL, then behave like kfree
  if (size == 0) {
    kfree(ptr);
    return NULL;
  }

  size = ALIGN(size);
  MemoryBlock *block = (MemoryBlock *)ptr - 1;

  // if current block is big enough, return it as is
  if (block->size >= size) {
    return ptr;
  }

  // if current block is not big enough, try to combine it with the next block
  if (block->next && block->next->free &&
      (block->size + Block_SIZE + block->next->size) >= size) {

    size_t combined_size = block->size + Block_SIZE + block->next->size;
    MemoryBlock *next_next_block = block->next->next;

    // allocate a new block and free the old one
    if (combined_size >= (size + Block_SIZE + ALIGNMENT)) {
      MemoryBlock *new_block =
          (MemoryBlock *)((uint8_t *)block + Block_SIZE + size);
      new_block->size = combined_size - size - Block_SIZE;
      new_block->free = 1;

      if (next_next_block && next_next_block->free) {
        new_block->size += Block_SIZE + next_next_block->size;
        new_block->next = next_next_block->next;
      } else {
        new_block->next = next_next_block;
      }

      block->size = size;
      block->next = new_block;
    } else {
      block->size = combined_size;
      block->next = next_next_block;
    }
    return ptr;
  }

  // no space to expand in-place, allocate a completely new block
  void *new_ptr = kmalloc(size);
  if (!new_ptr) {
    return NULL; // out of memory, original pointer remains unchanged
  }

  // copy old data to new memory
  memcpy(new_ptr, ptr, block->size);

  // free the old block
  kfree(ptr);

  return new_ptr;
}

void kfree(void *ptr) {
  if (!ptr) {
    return;
  }

  // Get the header by backing up from the data pointer
  MemoryBlock *block = (MemoryBlock *)ptr - 1;
  block->free = 1;

  // Find the block immediately preceding our freed block
  MemoryBlock *prev = NULL;
  MemoryBlock *current = free_list_head;

  while (current && current != block) {
    prev = current;
    current = current->next;
  }

  // Coalesce with the NEXT block if it is free
  if (block->next && block->next->free) {
    block->size += Block_SIZE + block->next->size;
    block->next = block->next->next;
  }

  // Coalesce with the PREVIOUS block if it exists and is free
  if (prev && prev->free) {
    prev->size += Block_SIZE + block->size;
    prev->next = block->next;
  }
}
