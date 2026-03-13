#include "fat32.h"
#include "vfs.h"

void vfs_init()
{
    fat32_init();
}

void vfs_ls()
{
    fat32_list_root();
}

void vfs_cat(const char* name)
{
    fat32_cat(name);
}

void vfs_write(const char* name, uint8_t* data, uint32_t size) {
    fat32_write_file(name, data, size);
}

void vfs_rm(const char* name) {
    fat32_delete_file(name);
}

void vfs_usage() {
    fat32_show_usage();
}
