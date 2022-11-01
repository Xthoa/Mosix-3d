#include "types.h"
#include "vfs.h"
#include "kheap.h"

PRIVATE Filesystem* fsroot;
PRIVATE Superblock* sbroot;
PRIVATE Node* root;

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
    child->sb = father->sb;
}
PRIVATE void del_node_child(Node* child){
	Node* f = child->father;
	if(f->child == child) f->child = child->next;
	else child->prev->next = child->next;
	if(child->next) child->next->prev = child->prev;
}
PRIVATE Node* create_subnode(Node* father, const char* name, u32 flag){
    Node* child = alloc_node(name, flag);
    add_node_child(father, child);
	if(child->sb){
		child->fops = child->sb->fops;
		child->nops = child->sb->nops;
    }
	return child;
}
PRIVATE void destroy_subnode(Node* child){
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
	return fs->read_sb(dev);
}

Node* find_node_in(Node* r, char* name){
	for(Node* o = r->child; o; o = o->next){
		if(o->nops->compare){
			if(!o->nops->compare(o, name)){
				return o;
			}
		}
		if(!strcmp(o->name, name)){
			return o;	// Already in node tree
		}
	}
	Node* d = r->nops->lookup(r, name);
	if(d) add_node_child(r, d);	// don't add if not found
	return d;
}
PUBLIC Node* path_walk(const char* name){
	char* str = kheap_clonestr(name);
	int slen = kstrlen(str);
	int i,j;
	Node* o = root;
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

PUBLIC File* open(char* path, int flag){
	Node* node = path_walk(path);
	if(!node) return -2;
	if(node->attr & INODE_DIRECTORY) return -1;
	File* f = kheap_alloc(sizeof(File));
	f->node = node;
	f->mode = flag;
	f->fops = node->fops;
	if(f->fops && f->fops->open) f->fops->open(node, f);
	return f;
}
PUBLIC void close(File* f){
	if(f->fops && f->fops->close) f->fops->close(f->node, f);
	kheap_free(f);
}
PUBLIC int read(File* f, char* buf, size_t size){
	if(f->fops && f->fops->read)
		return f->fops->read(f, buf, size, f->off);
	return -1;
}
PUBLIC int write(File* f, char* buf, size_t size){
	if(f->fops && f->fops->write)
		return f->fops->write(f, buf, size, f->off);
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
    create_subnode(root, "config", 0);
    create_subnode(root, "run", 0);
    Node* files = create_subnode(root, "files", 0);
    create_subnode(files, "boot", 0);
}
