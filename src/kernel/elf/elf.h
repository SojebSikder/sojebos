#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#define ELF_MAGIC 0x464C457F // ELF magic number -> "\x7FELF"

// ELF type
#define ET_EXEC 2

// ELF program header type
#define PT_LOAD 1

// ELF program header flags
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

typedef struct {
  uint8_t e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry; // entry point virtual address
  uint32_t e_phoff; // program header table offset
  uint32_t e_shoff; // section header table offset
  uint32_t e_flags;
  uint32_t e_ehsize; // ELF header size
  uint16_t e_phentsize;
  uint16_t e_phnum; // number of program headers
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx; // section header string table index
} __attribute__((packed)) elf_header_t;

typedef struct {
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr; // virtual address to map segment to
  uint32_t p_paddr;
  uint32_t p_filesz; // size of data in the file
  uint32_t p_memsz;  // size of data in memory (padded with zero if > filesz)
  uint32_t p_flags;
  uint32_t p_align;
} __attribute__((packed)) elf_program_header_t;

#endif
