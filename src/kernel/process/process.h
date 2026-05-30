#ifndef PROCESS_H
#define PROCESS_H

#include "../elf/elf.h"
#include <stdint.h>

#define MAX_PROCESS_NAME 32

typedef enum {
  PROCESS_STATE_READY,
  PROCESS_STATE_RUNNING,
  PROCESS_STATE_BLOCKED,
  PROCESS_STATE_TERMINATED
} process_state_t;

typedef struct process {
  uint32_t pid;                // unique process ID
  char name[MAX_PROCESS_NAME]; // process name/binary path
  process_state_t state;       // current execution state

  // architecture specific context
  uint32_t entry_point;    // memory address of executable entry point
  uint32_t user_stack_top; // top address of allocated user stack
  uint32_t page_directory; // CR3 value (physical address of page directory)

  struct process *next; // pointer for scheduler tracking linked list
} process_t;

/**
 * Parses an ELF file from the VFS, sets up address mapping, and runs it.
 * @param filename The path to the executable file inside the VFS.
 * @return int 0 on success, negative error code on failure.
 */
int exec_elf(const char *filename);

/**
 * Performs the low-level architecture switch to Ring 3.
 * Implemented in assembly.
 * @param entry_point The virtual address to jump to (EIP).
 * @param user_stack The top virtual address of the user stack (ESP).
 */
extern void enter_user_mode(uint32_t entry_point, uint32_t user_stack);

#endif
