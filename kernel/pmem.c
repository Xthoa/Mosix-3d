#include "types.h"
#include "pmem.h"
#include "boot.h"
#include "string.h"
#include "kheap.h"
#include "macros.h"

PRIVATE Freelist phyflist;

// Init freelist allocator
void pmem_init(BootArguments* bargs){
    phyflist.root = PmemExtentRoot;
    phyflist.max = 112; // 0x700 / 16 = 112
    phyflist.size = 0;
	init_spinlock(&phyflist.lock);

    ArdsBlock* ards = bargs->ards;
    for(int i = 0; i < bargs->ardscnt; i++){
        ArdsBlock* ab = ards + i;
        if(ab->type == ArdsAvailable){
            if(ab->base == 0) continue;
            // For compatibility, all x86s set the
            // first ards block as 0/0x9f000/1
            // We reserve the lowest 1M so this is the simplest way

            Extent* e = phyflist.root + phyflist.size;
            e->pos = ab->base / PAGE_SIZE;
            e->size = ab->len / PAGE_SIZE;
            phyflist.size ++;
        }
    }
}

// alloc&dealloc on general Freelist
PUBLIC u64 flist_alloc(Freelist *aloc,u32 size){
	acquire_spin(&aloc->lock);
	u64 addr=0;
	for(int i = 0; i < aloc->size; i++){
		Extent* f= aloc->root + i;
		if(f->size==size){
			addr = f->pos;
			pull_back_array(aloc->root, aloc->size, i, Extent);
			aloc->size --;
			break;
		}
        if(f->size>size){
			addr = f->pos;
			f->pos += size;
			f->size -= size;
			break;
		}
	}
	release_spin(&aloc->lock);
	return addr;
}
PUBLIC void flist_dealloc(Freelist *aloc,u64 addr,u32 size){
	acquire_spin(&aloc->lock);
	u32 i,len;
	for(i = 0; i < aloc->size; i++){
		Extent* f= aloc->root + i;
		//ASSERT_ARG(f->pos != addr, "%q %b", f->pos, f->size == size);
		if(f->pos >= addr+size){
			push_back_array(aloc->root, aloc->size, i, Extent);
			f->pos = addr;
			f->size = size;
			aloc->size ++;
			break;
		}
	}
	if(i == aloc->size){
		Extent* f=&(aloc->root[i]);
		f->pos = addr;
		f->size = size;
		aloc->size ++;
	}

	Extent* self = aloc->root + i;
	Extent* front = self - 1;
	Extent* end = self + 1;

	if(i !=0 && front->pos + front->size == addr){
		front->size += size;
		pull_back_array(aloc->root, aloc->size, i, Extent);
		end = self; self = front; i--;
        aloc->size--;
	}
	if(i != aloc->size - 1 && end->pos == addr + size){
		self->size += end->size;
		pull_back_array(aloc->root, aloc->size, i+1, Extent);
		aloc->size--;
	}
	release_spin(&aloc->lock);
	return;
}

u32 total_avail(Freelist* fl){
	acquire_spin(&fl->lock);
	u32 total = 0;
	for(int i = 0; i < fl->size; i++){
		Extent* e = fl->root + i;
		total += e->size;
		printk("%B %q %q\n",i, e->pos, e->size);
	}
	release_spin(&fl->lock);
	return total;
}
u32 total_phy_avail(){
	return total_avail(&phyflist);
}

// General interface on physical memory management
PUBLIC paddr_t alloc_phy(uint32_t pages){
    return flist_alloc(&phyflist, pages) * PAGE_SIZE;
}
PUBLIC void free_phy(paddr_t addr, uint32_t pages){
    flist_dealloc(&phyflist, addr / PAGE_SIZE, pages);
}