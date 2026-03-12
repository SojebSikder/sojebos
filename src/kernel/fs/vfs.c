#include "fat32.h"

void vfs_init()
{
    fat32_init();
}

void vfs_ls()
{
    fat32_list_root();
}
