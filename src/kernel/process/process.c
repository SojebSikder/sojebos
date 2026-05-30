#include "process.h"
#include "../fs/vfs.h"
#include "../libc/string.h"
#include "../paging/paging.h"

// Defined in assembly (see step 3)
extern void enter_user_mode(uint32_t entry_point, uint32_t user_stack);

int exec_elf(const char *filename) {
  // Open the file via VFS
  vfs_node_t *file = vfs_open(filename);
  if (!file) {
    return -1; // File not found
  }

  // Read and validate ELF Header
  elf_header_t elf_header;
  vfs_read(file, 0, sizeof(elf_header_t), (uint8_t *)&elf_header);

  uint32_t *magic = (uint32_t *)elf_header.e_ident;
  if (*magic != ELF_MAGIC) {
    vfs_close(file);
    return -2; // Not a valid ELF file
  }

  if (elf_header.e_type != ET_EXEC) {
    vfs_close(file);
    return -3; // Not an executable
  }

  // Parse Program Headers & Allocate Memory
  for (int i = 0; i < elf_header.e_phnum; i++) {
    elf_program_header_t ph;
    uint32_t offset = elf_header.e_phoff + (i * elf_header.e_phentsize);
    vfs_read(file, offset, sizeof(elf_program_header_t), (uint8_t *)&ph);

    // Only load segments flagged as PT_LOAD
    if (ph.p_type == PT_LOAD) {
      // Allocate physical pages and map them to ph.p_vaddr
      // Adjust this line depending on your exact page allocation API:
      // e.g., alloc_pages(ph.p_vaddr, ph.p_memsz, PAGE_USER | PAGE_WRITABLE);
      paging_map_user_memory(ph.p_vaddr, ph.p_memsz);

      // Zero out memory segment completely first (clears .bss automatically)
      memset((void *)ph.p_vaddr, 0, ph.p_memsz);

      // Copy segment payload from file into virtual memory
      if (ph.p_filesz > 0) {
        vfs_read(file, ph.p_offset, ph.p_filesz, (void *)ph.p_vaddr);
      }
    }
  }

  // Setup User Stack
  // We choose an arbitrary safe higher-half location for user stack, e.g.,
  // 0xBFFFF000
  uint32_t user_stack_top = 0xBFFFF000;
  uint32_t stack_size = 4096 * 4; // 16KB stack
  uint32_t user_stack_bottom = user_stack_top - stack_size;

  paging_map_user_memory(user_stack_bottom, stack_size);
  memset((void *)user_stack_bottom, 0, stack_size);

  vfs_close(file);

  // Privilege Context Switch
  // Use assembly stub to jump to Ring 3
  enter_user_mode(elf_header.e_entry, user_stack_top);

  return 0; // Should never actually be reached
}
