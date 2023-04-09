#pragma once

#include "types.h"
#include "spin.h"

#define NODE_DIRECTORY 1
#define NODE_MOUNTED 2
#define NODE_EXPIRED 4  // can be flushed
#define NODE_HARDLINK 8

#define SB_NO_HARDLINK 1
#define SB_READONLY 2
#define SB_RAM 4

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct s_Filesystem;
struct s_Superblock;
struct s_File;
struct s_FileOperations;
struct s_Node;
struct s_NodeOperations;
struct s_Mount;

typedef struct s_Node{
    char* name;     // name of the node
    struct s_Superblock* sb;
    struct s_FileOperations* fops;
    struct s_NodeOperations* nops;
    struct s_Node* father;  // tree linker
    struct s_Node* child;
    struct s_Node* prev;
    struct s_Node* next;
    u64 size;   // generally the size of the file
    u64 ctime;  // change (fmeta) time
    u64 atime;  // access time
    u64 mtime;  // modify time
    u64 attr;   // attributes
    u64* data;  // private data of fs
} Node;
typedef struct s_NodeOperations{
    Node* (*lookup)(Node* parent, char* name);
    int (*compare)(Node* self, char* name); // 0 on success
    int (*create)(Node* parent, Node* child, int mode);
    int (*delete)(Node* child);
    int (*mkdir)(Node* parent, Node* child, int mode);
    int (*rmdir)(Node* child);
    /*int (*rename)(Inode* inode, Dentry* dentry, Inode* newi, Dentry* newd);
    int (*getattr)(Dentry* dentry, u64* attr);
    int (*setattr)(Dentry* dentry, u64* attr);*/
} NodeOperations;

typedef struct s_File{
    Node* node;    // which actual file
    struct s_FileOperations* fops; // shortcut of operations
    u64 size;
    u64 off;      // generally offset of r/w pointer
    u32 mode;
    u32 href;    // handle reference count
    u64* data;
} File;
typedef struct s_FileOperations{
    int (*open)(Node* inode, File* file);
    int (*close)(Node* inode, File* file);
    int (*read)(File* file, char* buf, size_t size);
    int (*write)(File* file, char* buf, size_t size);
    int (*lseek)(File* file, u64 offset, u64 origin);
    int (*ioctl)(Node* inode, File* file, int cmd, int arg);
} FileOperations;

typedef struct s_Filesystem{
    struct s_Filesystem* prev;  // list
    struct s_Filesystem* next;
    char* name;
    struct s_Superblock* instance;
    struct s_Superblock* (*mount_sb)(Node* dev);
    void (*update_sb)(struct s_Superblock* sb);
    void (*release_sb)(struct s_Superblock* sb, struct s_Mount* mnt);
} Filesystem;

typedef struct s_Superblock{
    struct s_Superblock* prev;
    struct s_Superblock* next;
    Filesystem* type;
    FileOperations* fops;
    NodeOperations* nops;
    Node* root;
    Node* dev;
    struct s_Mount* mounts;
    u64 attr;
    u64* data;
    Spinlock mntlock;
} Superblock;

typedef struct s_Mount{
    struct s_Mount* father;
    struct s_Mount* prev;
    struct s_Mount* next;
    Superblock* sb;
    Node* sbroot;
    Node* origin;
    Node* mntpoint;
} Mount;

typedef struct s_Path{
    Mount* mnt;
    Node* node;
} Path;

Node* alloc_node(const char* name, u32 flag);

Filesystem* create_fstype(const char* name);
Filesystem* find_fstype(const char* name);
void destroy_fstype(Filesystem* tp);

Superblock* create_superblock(Filesystem* tp);
void destroy_superblock(Superblock* sb);

Node* create_subnode(Node* father, const char* name, u32 flag);
void destroy_subnode(Node* child);
Node* create_subfile(Node* father, char* name, u32 flag);
Node* create_subdir(Node* father, char* name, u32 flag);
void destroy_subfile(Node* n);
void destroy_subdir(Node* n);

Superblock* mount_sb(char* fstype, Node* dev);
void unmount_sb(Superblock* sb, Mount* m);
int mount_on(char* path, char* fstype, Node* dev);
int unmount_from(char* path);

void find_node_from(Path* croot, const char* name);
Path path_walk(const char* name);
void path_stringify(Path path, char* buf);

File* open(char* path, int flag);
File* open_node(Node* node, int flag);
void close(File* f);
int read(File* f, char* buf, size_t size);
int write(File* f, char* buf, size_t size);
int lseek(File* f, off_t off, int origin);
