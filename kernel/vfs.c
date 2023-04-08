#include "types.h"
#include "vfs.h"
#include "kheap.h"
#include "proc.h"

PRIVATE Filesystem* fsroot;
PRIVATE Superblock* sbroot;
PRIVATE Node* root;

Node* path_walk_stepback(const char* name, Bool* backed);

// Raw Node structure alloc/free
PUBLIC Node* alloc_node(const char* name, u32 flag){
    Node* n = kheap_alloc_zero(sizeof(Node));
    n->name = kheap_clonestr(name);
    n->attr = flag;
    return n;
}
PRIVATE void free_node(Node* n){
    kheap_freestr(n->name);
    kheap_free(n);
}
PRIVATE void add_node_child(Node* father, Node* child){
	child->father = father;
	Node* c = father->child;
	child->next = c;
	if(c) c->prev = child;
	father->child = child;
	child->prev = NULL;
    child->sb = father->sb;
}
PRIVATE void del_node_child(Node* child){
	Node* f = child->father;
	if(f->child == child) f->child = child->next;
	else child->prev->next = child->next;
	if(child->next) child->next->prev = child->prev;
}
PUBLIC Node* create_subnode(Node* father, const char* name, u32 flag){
    Node* child = alloc_node(name, flag);
    add_node_child(father, child);
	if(child->sb){
		child->fops = child->sb->fops;
		child->nops = child->sb->nops;
    }
	else{
		child->fops = father->fops;
		child->nops = father->nops;
	}
	return child;
}
PUBLIC void destroy_subnode(Node* child){
    del_node_child(child);
    free_node(child);
}

// About filesystems: create/find/destroy
PUBLIC Filesystem* create_fstype(const char* name){
	Filesystem* tp=kheap_alloc(sizeof(Filesystem));
	tp->name = kheap_clonestr(name);
	tp->prev = NULL;
	if(fsroot){
		tp->next = fsroot;
		fsroot->prev = tp;
	}
	fsroot = tp;
	return tp;
}
PUBLIC Filesystem* find_fstype(const char* name){
	for(Filesystem* t = fsroot; t; t = t->next){
		if(!strcmp(t->name, name)) return t;
	}
	return NULL;
}
PUBLIC void destroy_fstype(Filesystem* tp){
	if(fsroot == tp) fsroot = tp->next;
	else tp->prev->next = tp->next;
	if(tp->next) tp->next->prev = tp->prev;
	kheap_freestr(tp->name);
	kheap_free(tp);
}

// About superblocks
PUBLIC Superblock* create_superblock(Filesystem* tp){
	Superblock* sb = kheap_alloc(sizeof(Superblock));
	sb->type = tp;
	if(sbroot){
		sb->next = sbroot;
		sbroot->prev = sb;
	}
	sbroot = sb;
	return sb;
}
PUBLIC void destroy_superblock(Superblock* sb){
	if(sbroot == sb) sbroot = sb->next;
	else sb->prev->next = sb->next;
	if(sb->next) sb->next->prev = sb->prev;
	kheap_free(sb);
}
PUBLIC Superblock* mount_sb(char* fstype, Node* dev){
	Filesystem* fs = find_fstype(fstype);
	if(!fs) return NULL;
	return fs->mount_sb(dev);
}
PUBLIC void unmount_sb(Superblock* sb){
	Filesystem* fs = sb->type;
	if(!fs->release_sb) return;
	fs->release_sb(sb);
}
PRIVATE void mount_on_node(Node* point, char* fstype, Node* dev){
	Superblock* sb = mount_sb(fstype, dev);
	point->attr |= NODE_MOUNTED;
	point->sb = sb;
	point->child = sb->root;
}
PRIVATE void unmount_from_node(Node* point){
	Superblock* sb = point->sb;
	unmount_sb(sb);
}
PUBLIC int mount_on(char* path, char* fstype, Node* dev){
	Node* point = path_walk(path);
	if(!point) return ErrNull;
	mount_on_node(point, fstype, dev);
	return 0;
}
PUBLIC int unmount_from(char* path){
	Node* point = path_walk(path);
	if(!point) return ErrNull;
	unmount_from_node(point);
	return 0;
}

Node* find_node_in(Node* r, char* name){
	for(Node* o = r->child; o; o = o->next){
		if(o->nops && o->nops->compare){
			if(!o->nops->compare(o, name)){
				if(o->attr & NODE_MOUNTED) o = o->child;
				return o;
			}
		}
		if(!strcmp(o->name, name)){
			if(o->attr & NODE_MOUNTED) o = o->child;
			return o;	// Already in node tree
		}
	}
	if(!r->nops || !r->nops->lookup) return NULL;
	Node* d = r->nops->lookup(r, name);
	return d;
}
PUBLIC Node* find_node_from(Node* croot, const char* name){
	char* str = kheap_clonestr(name);
	int slen = kstrlen(str);
	int i,j;
	Node* o = croot;
	for(i = 0, j = 0; str[i]; i++){
		if(str[i] == '/'){
			str[i]=0;
			o = find_node_in(o, str + j);
			if(!o)return NULL;
			j = i + 1;
		}
	}
	o = find_node_in(o, str + j);
	kheap_freestr(str);
	return o;
}
PUBLIC Node* path_walk(const char* name){
	if(name[0] == '/') name = name + 1;
	if(name[0] == '\0') return root;
	return find_node_from(root, name);
}

PUBLIC File* open(char* path, int flag){
	Node* node = path_walk(path);
	if(!node) return NULL;
	if(node->attr & NODE_DIRECTORY) return NULL;
	return open_node(node, flag);
}
File* open_node(Node* node, int flag){
	File* f = kheap_alloc(sizeof(File));
	f->node = node;
	f->mode = flag;
	f->off = 0;
	f->href = 0;
	f->fops = node->fops;
	f->size = node->size;
	if(f->fops && f->fops->open) f->fops->open(node, f);
	return f;
}
PUBLIC void close(File* f){
	if(f->fops && f->fops->close) f->fops->close(f->node, f);
	kheap_free(f);
}
PUBLIC int read(File* f, char* buf, size_t size){
	if(f->fops && f->fops->read)
		return f->fops->read(f, buf, size);
	return -1;
}
PUBLIC int write(File* f, char* buf, size_t size){
	if(f->fops && f->fops->write)
		return f->fops->write(f, buf, size);
	return -1;
}
PUBLIC int lseek(File* f, off_t off, int origin){
	if(f->fops && f->fops->lseek)
		return f->fops->lseek(f, off, origin);
	if(origin == SEEK_SET){
		if(off > f->size) return -1;
		f->off = off;
	}
	elif(origin == SEEK_CUR){
		if(f->off+off > f->size) return -1;
		f->off += off;
	}
	elif(origin == SEEK_END){
		if(off > f->size) return -1;
		f->off = f->size - off;
	}
	else return -1;
}

void vfs_init(){
    root = alloc_node("", 0);
	root->father = root;
    create_subnode(root, "config", 0);
    create_subnode(root, "run", 0);
    Node* files = create_subnode(root, "files", 0);
    create_subnode(files, "boot", 0);
}
