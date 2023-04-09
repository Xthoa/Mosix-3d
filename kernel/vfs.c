#include "types.h"
#include "vfs.h"
#include "kheap.h"
#include "proc.h"

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
Node* create_subfile(Node* father, char* name, u32 flag){
	Node* n = create_subnode(father, name, flag);
	if(n->nops && n->nops->create) n->nops->create(father, n, flag);
	return n;
}
Node* create_subdir(Node* father, char* name, u32 flag){
	Node* n = create_subnode(father, name, flag | NODE_DIRECTORY);
	create_subnode(n, ".", NODE_DIRECTORY | NODE_HARDLINK);
	n->child = n;
	create_subnode(n, "..", NODE_DIRECTORY | NODE_HARDLINK);
	n->child = father;
	if(n->nops && n->nops->mkdir) n->nops->mkdir(father, n, flag);
	return n;
}
void destroy_subfile(Node* n){
	if(n->nops && n->nops->delete) n->nops->delete(n);
	destroy_subnode(n);
}
void destroy_subdir(Node* n){
	if(n->nops && n->nops->rmdir) n->nops->rmdir(n);
	destroy_subnode(n);
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
	sb->mounts = NULL;
	sb->type = tp;
	if(sbroot){
		sb->next = sbroot;
		sbroot->prev = sb;
	}
	sbroot = sb;
	init_spinlock(&sb->mntlock);
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
PUBLIC void unmount_sb(Superblock* sb, Mount* m){
	Filesystem* fs = sb->type;
	if(!fs->release_sb) return;
	fs->release_sb(sb, m);
}
PRIVATE void mount_on_node(Path point, char* fstype, Node* dev){
	Superblock* sb = mount_sb(fstype, dev);
	Node* n = point.node;
	Mount* m = kheap_alloc(sizeof(Mount));
	m->sb = sb;
	m->father = point.mnt;
	acquire_spin(&sb->mntlock);
	m->next = sb->mounts;
	if(m->next) m->next->prev = m;
	m->prev = NULL;
	sb->mounts = m;
	release_spin(&sb->mntlock);
	m->mntpoint = n;
	m->origin = n->child;
	m->sbroot = sb->root;
	n->attr |= NODE_MOUNTED;
	n->child = m;
}
PRIVATE void unmount_from_node(Path point){
	Node* n = point.node;
	Mount* m = n->child;
	Superblock* sb = m->sb;
	n->attr &= ~NODE_MOUNTED;
	n->child = m->origin;
	acquire_spin(&sb->mntlock);
	if(m->prev) m->prev->next = m->next;
	else sb->mounts = m->next;
	if(m->next) m->next->prev = m->prev;
	release_spin(&sb->mntlock);
	unmount_sb(sb, m);
	kheap_free(m);
}
PUBLIC int mount_on(char* path, char* fstype, Node* dev){
	Path point = path_walk(path);
	if(!point.node) return ErrNull;
	mount_on_node(point, fstype, dev);
	return 0;
}
PUBLIC int unmount_from(char* path){
	Path point = path_walk(path);
	if(!point.node) return ErrNull;
	unmount_from_node(point);
	return 0;
}

void follow_link(Path* p){
	Node* o = p->node;
	if(o->attr & NODE_HARDLINK){
		p->node = o = o->child;
	}
	if(o->attr & NODE_MOUNTED){
		Mount* m = o->child;
		p->mnt = m;
		p->node = m->sbroot;
	}
}
void find_node_in(Path* p, char* name){
	Node* r = p->node;
	for(Node* o = r->child; o; o = o->next){
		if(o->nops && o->nops->compare){
			if(!o->nops->compare(o, name)){
				p->node = o;
				follow_link(p);
				return;
			}
		}
		if(!strcmp(o->name, name)){
			// Already in node tree
			p->node = o;
			follow_link(p);
			return;
		}
	}
	if(!r->nops || !r->nops->lookup){
		p->node = NULL;
		return;
	}
	p->node = r->nops->lookup(r, name);
}
void find_node_from(Path* croot, const char* name){
	char* str = kheap_clonestr(name);
	int slen = kstrlen(str);
	int i, j;
	for(i = 0, j = 0; str[i]; i++){
		if(str[i] == '/'){
			str[i]=0;
			find_node_in(croot, str + j);
			if(!croot->node) return;
			j = i + 1;
		}
	}
	find_node_in(croot, str + j);
	kheap_freestr(str);
}
Path path_walk(const char* name){
	Path p = {NULL, root};
	if(name[0] == '/') name = name + 1;
	if(name[0] == '\0') return p;
	find_node_from(&p, name);
	return p;
}

void path_stringify(Path path, char* buf){
	Node** nl = kheap_alloc(sizeof(Node*) * 16);
	Node* n = path.node;
	int i = 0;
	while(True){
		if(n == n->father){
			if(path.mnt == NULL) break;
			n = path.mnt->mntpoint;
			path.mnt = path.mnt->father;
		}
		nl[i++] = n;
		n = n->father;
	}
	int j = 0;
	if(i == 0) buf[j++] = '/';
	while(i > 0){
		buf[j++] = '/';
		Node* n = nl[--i];
		strcpy(buf + j, n->name);
		j += kstrlen(n->name);
	}
	buf[j] = '\0';
}

PUBLIC File* open(char* path, int flag){
	Node* node = path_walk(path).node;
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
    root = alloc_node("/", 0);
	root->father = root;
    create_subdir(root, "config", 0);
    create_subdir(root, "run", 0);
    Node* files = create_subdir(root, "files", 0);
    create_subdir(files, "boot", 0);
}
