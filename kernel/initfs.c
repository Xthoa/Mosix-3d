#include "initfs.h"

PRIVATE Filesystem* type;

Superblock* initfs_read_sb(Node* dev){
    Superblock* sb = create_superblock(type);
    sb->root = alloc_node("/", 0);
    return sb;
}

void mount_initfs(){
    type = create_fstype("initfs");
    type->read_sb = initfs_read_sb;
}