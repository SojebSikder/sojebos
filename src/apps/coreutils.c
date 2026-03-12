#include "../kernel/fs/vfs.h"
#include "../drivers/console.h"


void ls_app() {
    vfs_ls();
}

void cat_app(int argc, char *argv[]) {
    if(argc < 2){
        console_print("Usage: cat <filename>\n");
        return;
    }
    vfs_cat(argv[1]);
}
