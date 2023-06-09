#include "initfs.h"
#include "kheap.h"
#include "string.h"
#include "asm.h"

PRIVATE Filesystem* type;
PRIVATE NodeOperations* nops;
PRIVATE FileOperations* fops;
PRIVATE Superblock* sb;

PRIVATE Superblock* initfs_mount_sb(Node* dev){
	if(sb) return sb;
	sb = create_superblock(type);
	sb->nops = nops;
	sb->fops = fops;
	Node* n = sb->root = alloc_node("/", NODE_DIRECTORY);
	n->father = n;
	n->data = ARCHIVE_ADDR;
	n->sb = sb;
	n->nops = nops;
	n->fops = fops;
	Node* d = create_subdir(n, ".", NODE_DIRECTORY | NODE_HARDLINK);
	d->child = n;
	d = create_subdir(n, "..", NODE_DIRECTORY | NODE_HARDLINK);
	d->child = n;
	return sb;
}

u64 oct2bin(char *octa){
	u64 ret=0;
	while(*octa && *octa!=' '){
		ret<<=3;
		ret+=*(octa++)-'0';
	}
	return ret;
}
PRIVATE Node* initfs_lookup(Node* parent, char* name){
	TarMetadata* meta = parent->data;
	while (!memcmp(meta->ustar, "ustar", 5)) {
		if(meta->isize == 0) meta->isize = oct2bin(meta->size);
		if (!strcmp(meta->name, name)) {
			Node* n = create_subnode(parent, meta->name, 0);
			n->size = meta->isize;
			n->data = meta + 1;
			return n;
		}
		int secs = (meta->isize + 0x1ff) >> 9;
		meta += secs + 1;
	}
	return NULL;
}

PRIVATE int initfs_open(Node* node, File* file){
	if(file->mode & OPEN_DIR) file->off = 0;
	file->data = node->data;
	return 0;
}
PRIVATE int initfs_read(File* f, char* buf, size_t size){
	if(f->off + size > f->size) size = f->size - f->off;
	memmove(buf, (u8*)f->data + f->off, size);
	f->off += size;
	return size;
}
PRIVATE int initfs_write(File* f, char* buf, size_t size){
	return -1;
}
PRIVATE int initfs_iterate(File* f, char** buf, size_t count){
	if(count == 0) return 0;
	Node* p = f->node;
	int i = 0;
	if(f->off == 0){
		buf[i++] = kheap_clonestr(".");
		f->off ++;
		if(i == count) return i;
	}
	if(f->off == 1){
		buf[i++] = kheap_clonestr("..");
		f->off = f->data;
		if(i == count) return i;
	}
	TarMetadata* meta = f->off;
	while (!memcmp(meta->ustar, "ustar", 5)) {
		if(i == count) break;
		if(meta->isize == 0) meta->isize = oct2bin(meta->size);
		buf[i++] = kheap_clonestr(meta->name);
		int secs = (meta->isize + 0x1ff) >> 9;
		meta += secs + 1;
	}
	f->off = meta;
	return i;
}

void mount_initfs(){
	type = create_fstype("initfs");
	type->mount_sb = initfs_mount_sb;
	sb = NULL;

	nops = kheap_alloc(sizeof(NodeOperations));
	nops->compare = NULL;
	nops->lookup = initfs_lookup;

	fops = kheap_alloc(sizeof(FileOperations));
	fops->open = initfs_open;
	fops->close = NULL;
	fops->read = initfs_read;
	fops->write = initfs_write;		// delete it when we have FS.RO flag
	fops->ioctl = NULL;
	fops->lseek = NULL;
	fops->iterate = initfs_iterate;

	mount_on("/files/boot", "initfs", NULL);
}