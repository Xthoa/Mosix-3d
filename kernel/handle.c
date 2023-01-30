#include "handle.h"
#include "kheap.h"
#include "string.h"
#include "proc.h"
#include "vmem.h"
#include "vfs.h"
#include "macros.h"
#include "asm.h"
#include "exec.h"

void alloc_htab(Process* p){
    p->htab.table = malloc_page4k_attr(1, PGATTR_NOEXEC);
    p->htab.free = create_bitmap(MAX_HANDLE);
    init_spinlock(&p->htab.lock);
}
void free_htab(Process* p){
    free_page4k(p->htab.table, 1);
    destroy_bitmap(p->htab.free);
}
HandleTable* get_cur_htab(){
    Process* p = GetCurrentProcess();
    return &p->htab;
}
Handle* get_handle(int no){
    return get_cur_htab()->table + no;
}

int htab_insert(HandleTable* htab, void* obj, u32 type){
    acquire_spin(&htab->lock);
    int no = alloc_bit(htab->free);
    Handle* h = htab->table + no;
    h->ptr = obj;
    h->type = type;
    h->flag = 0;
    release_spin(&htab->lock);
    return no;
}
void htab_delete(HandleTable* htab, int no){
    acquire_spin(&htab->lock);
    Handle* h = htab->table + no;
    h->ptr = 0;
    free_bit(htab->free, no);
    release_spin(&htab->lock);
}

// TODO: parameter check
int hop_openfile(char* path, int flag){
    File* f = open(path, flag);
    lock_inc(f->href);
    return htab_insert(get_cur_htab(), f, HANDLE_FILE);
}
int hop_readfile(int no, char* dst, uint64_t sz){
    Handle* h = get_handle(no);
    return read(h->ptr, dst, sz);
}
int hop_writefile(int no, char* src, uint64_t sz){
    Handle* h = get_handle(no);
    return write(h->ptr, src, sz);
}
void hop_closefile(int no){
    Handle* h = get_handle(no);
    File* f = h->ptr;
    lock_dec(f->href);
    if(f->href == 0) close(f);
    htab_delete(get_cur_htab(), no);
}

int hop_create_process(char* path){
    Process* p = ExecuteFile(path);
    lock_inc(p->href);
    return htab_insert(get_cur_htab(), p, HANDLE_PROCESS);
}
int hop_find_process(char* name){
    Process* p = find_process(name);
    lock_inc(p->href);
    return htab_insert(get_cur_htab(), p, HANDLE_PROCESS);
}
void hop_wait_process(int no){
    Handle* h = get_handle(no);
    wait_process(h->ptr);
}
void hop_close_process(int no){
    Handle* h = get_handle(no);
    Process* p = h->ptr;
    lock_dec(p->href);
    if(p->href == 0) reap_process(p);
    htab_delete(get_cur_htab(), no);
}

int hop_create_mutex(){
    Mutex* m = kheap_alloc(sizeof(Mutex));
    init_mutex(m);
    lock_inc(m->href);
    return htab_insert(get_cur_htab(), m, HANDLE_MUTEX);
}
void hop_close_mutex(int no){
    Handle* h = get_handle(no);
    kheap_free(h->ptr);
    htab_delete(get_cur_htab(), no);
}
