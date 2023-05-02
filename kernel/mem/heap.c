#include "kheap.h"
#include "vmem.h"
#include "pmem.h"
#include "vfs.h"
#include "handle.h"
#include "proc.h"
#include "heap.h"

void create_heap(size_t pages){
    Process* p = GetCurrentProcess();
    p->heap = kheap_alloc(sizeof(Freelist));
    init_spinlock(&p->heap->lock);
    Extent* e = p->heap->root = malloc_page4k(1);
    p->heap->max = 256;
    p->heap->size = 1;
    e->pos = DEFAULT_HEAPPOS;
    e->size = pages * PAGE_SIZE;
    paddr_t phy = alloc_phy(pages);
    insert_vmarea(p->vm, DEFAULT_HEAPPOS, phy, pages, VM_HEAP, VM_RWX);
    set_mappings(DEFAULT_HEAPPOS, phy, pages, 0);
}
void destroy_heap(){
    Process* p = GetCurrentProcess();
    if(!p->heap) return;
    free_page4k(p->heap->root, 1);
    kheap_free(p->heap);
}

PUBLIC void* heap_alloc(u32 size){
    Process* p = GetCurrentProcess();
    size = ((size+4)+7)/8*8;    // align address & size to 8bytes
    vaddr_t addr = flist_alloc(p->heap, size);
    *(u32*)addr = size;
    return addr + 4;
}
PUBLIC void* heap_alloc_zero(u32 size){
    void* addr = heap_alloc(size);
    memset(addr, 0, size);
    return addr;
}
PUBLIC void heap_free(void* ptr){
    Process* p = GetCurrentProcess();
    u32 size = *(u32*)((vaddr_t)ptr - 4);
    flist_dealloc(p->heap, (vaddr_t)ptr - 4, size);
}

Bool pf_commitmem(u64 cr2){
    Process* p = GetCurrentProcess();
    if(p->rsb < cr2 && p->rsb + p->sl * PAGE_SIZE > cr2){
        u64 phy = alloc_phy(1);
        set_mapping(align_4k(cr2), phy, PGATTR_NOEXEC);
        return True;
    }
    return False;
}
