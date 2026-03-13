#include "../drivers/console.h"
#include "../kernel/fs/vfs.h"
#include "../kernel/libc/string.h"
#include <stdint.h>

void ls_app() { vfs_ls(); }

void cat_app(int argc, char *argv[]) {
  if (argc < 2) {
    console_print("Usage: cat <filename>\n");
    return;
  }
  vfs_cat(argv[1]);
}

// write a new file
void write_app(int argc, char *argv[]) {
  if (argc < 3) {
    console_print("Usage: write <filename> <content>\n");
    return;
  }

  char *filename = argv[1];
  char *content = argv[2];
  uint32_t size = strlen(content);

  // write to vfs
  vfs_write(filename, (uint8_t *)content, size);
  console_print("File written successfully!\n");
}

// remove a file
void rm_app(int argc, char *argv[]) {
  if (argc < 2) {
    console_print("Usage: rm <filename>\n");
    return;
  }

  char *filename = argv[1];
  vfs_rm(filename);
  console_print("File deleted successfully!\n");
}
