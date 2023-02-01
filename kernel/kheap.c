#include "types.h"
#include "kheap.h"
#include "pmem.h"

PRIVATE Freelist kheapflist;

// Initialize kernel heap.
void kheap_init(){
    kheapflist.max = 128;
    kheapflist.size = 1;
    init_spinlock(&kheapflist.lock);
    Extent* r = kheapflist.root = KHEAP_EXTENT_ROOT;
    r->pos = KHEAP_BASE;
    r->size = KHEAP_INITIAL_SIZE;
}

// alloc and free from kernel heap
PUBLIC void* kheap_alloc(u32 size){
    size = ((size+4)+7)/8*8;    // align address & size to 8bytes
    vaddr_t addr = flist_alloc(&kheapflist, size);
    *(u32*)addr = size;
    return addr + 4;
}
PUBLIC void* kheap_alloc_zero(u32 size){
    void* addr = kheap_alloc(size);
    memset(addr, 0, size);
    return addr;
}
PUBLIC void kheap_free(void* ptr){
    u32 size = *(u32*)((vaddr_t)ptr - 4);
    flist_dealloc(&kheapflist, (vaddr_t)ptr - 4, size);
}

// clone and free strings which are temp or readonly
// so that we can operate on them
PUBLIC void* kheap_clonestr(char* str){
    uint len = strlen(str);
    ushort* ptr = kheap_alloc(len + 3);
    ptr[0] = len;       // use first 2 bytes to store length
    strcpy(ptr + 1, str);
    return ptr + 1;     // and return the addr of the string
}
PUBLIC void kheap_freestr(char* str){
    kheap_free(str-2);
}
PUBLIC uint16_t kstrlen(char* kstr){
    ushort* ptr = (short*)(kstr - 2);
    return ptr[0];
}

u32 total_kheap_avail(){
    return total_avail(&kheapflist);
}
