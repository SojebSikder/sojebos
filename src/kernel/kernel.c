#include "../apps/apps.h"
#include "../drivers/console.h"
#include "./fs/vfs.h"
#include "./libc/string.h"
#include "./shell/shell.h"
#include "gdt/gdt.h"
#include "gdt/tss.h"
#include "memory/memory.h"
#include <stddef.h>
#include <stdint.h>

// external symbol from linker script
extern uint8_t _kernel_end[];

void __attribute__((section(".text.kernel_main"))) kernel_main() {
  // Initialize Memory allocator first
  // Let's give the heap 4MB of space starting after kernel
  void *heap_start = (void *)_kernel_end;
  size_t heap_size = 4 * 1024 * 1024; // 4MB

  kheap_init(heap_start, heap_size);

  // Initialize GDT
  gdt_init();

  // Initialize TSS
  uint32_t kernel_stack_top;
  asm volatile("mov %%esp, %0" : "=r"(kernel_stack_top));
  tss_init(5, 0x10, kernel_stack_top);

  // init file system
  vfs_init();
  // print welcome message
  console_clear();
  console_print("\n");
  console_print("   _____   ____       _  ______ ____   ____   _____ \n");
  console_print("  / ____| / __ \\     | ||  ____|  _ \\ / __ \\ / ____|\n");
  console_print(" | (___  | |  | |    | || |__  | |_) | |  | | (___  \n");
  console_print("  \\___ \\ | |  | | _  | ||  __| |  _ <| |  | |\\___ \\ \n");
  console_print("  ____) || |__| || |_| || |____| |_) | |__| |____) |\n");
  console_print(" |_____/  \\____/  \\___/ |______|____/ \\____/|_____/ \n");
  console_print("\n");
  console_print("Welcome to SojebOS\n");

  // Register apps
  register_all_apps();

  // run shell
  shell_run();
}
