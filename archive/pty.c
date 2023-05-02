#include "vfs.h"
#include "kheap.h"
#include "macros.h"
#include "exec.h"
#include "string.h"
#include "msglist.h"
#include "proc.h"
#include "handle.h"
#include "asm.h"
#include "pty.h"

Node* ptsdir;
Freelist ptsfl;

int master_open(Node* node, File* file){
    Pty* pty = file->data = kheap_alloc(sizeof(Pty));
    pty->slaveno = flist_alloc(&ptsfl, 1);

    char* name = kheap_alloc(4);
    hex2str(hex2bcd(pty->slaveno), name);
    pty->slave = create_subnode(ptsdir, name, NODE_DEVICE | NODE_CHARDEV);
    kheap_free(name);
    pty->slave->data = pty;

    init_buffer(&pty->in, 1, 256, True, True);
    init_buffer(&pty->out, 1, 256, True, True);

    return 0;
}
int master_read(File* file, char* buf, size_t sz){
    Pty* pty = file->data;
    for(int i = 0; i < sz; i++){
        buf[i] = wait_read_buffer(&pty->out);
    }
    return sz;
}
int master_write(File* file, char* buf, size_t sz){
    Pty* pty = file->data;
    for(int i = 0; i < sz; i++){
        wait_write_buffer(&pty->in, buf[i]);
    }
    return sz;
}
int master_close(Node* node, File* file){
    Pty* pty = file->data;
    destroy_subnode(pty->slave);
    flist_dealloc(&ptsfl, pty->slaveno, 1);

    destroy_buffer(&pty->in);
    destroy_buffer(&pty->out);

    kheap_free(pty);
    return 0;
}

int slave_open(Node* node, File* file){
    file->data = node->data;
    return 0;
}
int slave_read(File* file, char* buf, size_t sz){
    Pty* pty = file->data;
    for(int i = 0; i < sz; i++){
        buf[i] = wait_read_buffer(&pty->in);
    }
    return sz;
}
int slave_write(File* file, char* buf, size_t sz){
    Pty* pty = file->data;
    for(int i = 0; i < sz; i++){
        wait_write_buffer(&pty->out, buf[i]);
    }
    return sz;
}
int slave_close(Node* node, File* file){
    return 0;
}

Export void entry(int status){
    if(status == DRIVER_EXIT) return;

    Node* devdir = path_walk("/run/dev").node;
    Node* ptmx = create_subnode(devdir, "ptmx", NODE_DEVICE | NODE_CHARDEV);
    ptsdir = create_subdir(devdir, "pts", 0);

    FileOperations* pmop = kheap_alloc(sizeof(FileOperations));
    pmop->open = master_open;
    pmop->close = master_close;
    pmop->read = master_read;
    pmop->write = master_write;
    pmop->lseek = NULL;
    pmop->ioctl = NULL;
    ptmx->fops = pmop;

    FileOperations* psop = kheap_alloc(sizeof(FileOperations));
    psop->open = slave_open;
    psop->close = slave_close;
    psop->read = slave_read;
    psop->write = slave_write;
    psop->lseek = NULL;
    psop->ioctl = NULL;
    ptsdir->fops = psop;

    ptsfl.root = kheap_alloc(256);
    ptsfl.max = 16;
    ptsfl.size = 1;
    ptsfl.root[0].pos = 1;
    ptsfl.root[0].size = 16;
    init_spinlock(&ptsfl.lock);
}