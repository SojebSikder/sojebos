extern void vfs_init();
extern void vfs_ls();

void ls_app() {
    vfs_init();
    vfs_ls();
}
