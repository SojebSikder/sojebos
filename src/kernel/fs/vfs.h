#include <stdint.h>

void vfs_init();
void vfs_ls();
void vfs_cat(const char* name);
//
// file write
//
void vfs_write(const char* name, uint8_t* data, uint32_t size);
void vfs_rm(const char* name);
