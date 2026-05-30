#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

#define VFS_FILE      0x01
#define VFS_DIRECTORY 0x02

struct vfs_node;

// Function pointer types for filesystem operations
typedef uint32_t (*vfs_read_type_t)(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
typedef uint32_t (*vfs_write_type_t)(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
typedef void (*vfs_close_type_t)(struct vfs_node *node);

// Abstract representation of an open file or directory instance
typedef struct vfs_node {
    char name[256];
    uint32_t flags;       // VFS_FILE or VFS_DIRECTORY
    uint32_t size;        // Size of file in bytes
    uint32_t inode;       // FS-specific identifier (cluster number for FAT)

    // Function pointers filled by the driver
    vfs_read_type_t  read;
    vfs_write_type_t write;
    vfs_close_type_t close;

    void *priv_data;      // Driver-specific scratchpad data
} vfs_node_t;

// Global VFS Layer API called by your Kernel / Shell / ELF Loader
void vfs_init(void);
vfs_node_t* vfs_open(const char *filename);
uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
void vfs_close(vfs_node_t *node);

// Directory wrappers
void vfs_ls(void);
void vfs_cd(const char *path);
void vfs_pwd(void);
void vfs_create_directory(const char *dirname);
void vfs_rm(const char *name);
void vfs_cat(const char *name);
void vfs_rmdir(const char *dirname);
void vfs_usage(void);
void vfs_write_temp(const char *name, uint8_t *data, uint32_t size);


#endif
