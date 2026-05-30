#include "vfs.h"
#include "../memory/memory.h" // Assuming kmalloc/kfree live here
#include "fat32.h"

// A reference pointer to our primary/root file system driver hook
static vfs_node_t *vfs_root = NULL;

void vfs_init(void) {
  fat32_init();
  // In a multi-device OS, you would mount devices here.
}

vfs_node_t *vfs_open(const char *filename) {
  // Allocate a generic tracking node
  vfs_node_t *node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
  if (!node)
    return NULL;

  // Query your FAT32 layer to see if the file exists and populate details.
  // fat32_lookup to fill driver specifics.
  if (fat32_file_lookup(filename, node) != 0) {
    kfree(node);
    return NULL;
  }

  // Node is now populated with FAT32 specifics and its specialized read/write
  // pointers
  return node;
}

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size,
                  uint8_t *buffer) {
  if (node && node->read) {
    return node->read(node, offset, size, buffer);
  }
  return 0;
}

uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size,
                   uint8_t *buffer) {
  if (node && node->write) {
    return node->write(node, offset, size, buffer);
  }
  return 0;
}

void vfs_write_temp(const char *name, uint8_t *data, uint32_t size) {
  fat32_write_file(name, data, size);
}


void vfs_close(vfs_node_t *node) {
  if (node) {
    if (node->close) {
      node->close(node);
    }
    kfree(node);
  }
}

// Global state/directory wrappers route straight down to current active
// directory tracking
void vfs_ls(void) { fat32_ls(); }
void vfs_cd(const char *path) { fat32_cd(path); }
void vfs_pwd(void) { fat32_pwd(); }
void vfs_create_directory(const char *dirname) {
  fat32_create_directory(dirname);
}
void vfs_rm(const char *name) { fat32_delete_file(name); }
void vfs_rmdir(const char *dirname) { fat32_rmdir(dirname); }
void vfs_cat(const char *name) { fat32_cat(name); }
void vfs_usage(void) { fat32_show_usage(); }
