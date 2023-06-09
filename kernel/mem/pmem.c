#include "types.h"
#include "pmem.h"
#include "boot.h"
#include "string.h"
#include "kheap.h"
#include "macros.h"
#include "asm.h"

u32 memtotal;
PRIVATE Freelist phyflist;

// Init freelist allocator
void pmem_init(BootArguments* bargs){
    phyflist.root = PmemExtentRoot;
    phyflist.max = 112; // 0x700 / 16 = 112
    phyflist.size = 0;
	init_spinlock(&phyflist.lock);
	memtotal = 0;

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

			memtotal += ab->len / PAGE_SIZE;
        }
    }
}

// alloc&dealloc on general Freelist
PUBLIC u64 flist_alloc(Freelist *aloc,u32 size){
	ASSERT_ARG(size != 0, "aloc=%p", aloc);
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
PUBLIC u64 flist_alloc_from(Freelist *aloc, u64 begin, u32 size){
	ASSERT_ARG(size != 0, "aloc=%p", aloc);
	acquire_spin(&aloc->lock);
	u64 addr = 0;
	for(int i = 0; i < aloc->size; i++){
		Extent* f = aloc->root + i;
		if(f->pos + f->size < begin) continue;
		if(f->pos < begin){
			u64 end = f->pos + size;
			if(end - begin == size){	// ****[**]
				f->size -= size;
				addr = begin;
				break;
			}
			if(end - begin > size){		// ***[**]***
				push_back_array(aloc->root, aloc->size, i, Extent);
				aloc->size ++;
				f->size = begin - f->pos;
				addr = begin;
				f = f + 1;
				f->pos = begin + size;
				f->size = end - f->pos;
				break;
			}
			continue;
		}
		if(f->size==size){		// [***]
			addr = f->pos;
			pull_back_array(aloc->root, aloc->size, i, Extent);
			aloc->size --;
			break;
		}
        if(f->size>size){		// [**]****
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
	ASSERT_ARG(size != 0, "aloc=%p", aloc);
	acquire_spin(&aloc->lock);
	u32 i,len;
	for(i = 0; i < aloc->size; i++){
		Extent* f= aloc->root + i;
		if(f->pos == addr)bochsdbg();
		ASSERT_ARG(f->pos != addr, "pos=%q samesize=%b", f->pos, f->size == size);
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
		//printk("%B %q %q\n",i, e->pos, e->size);
	}
	release_spin(&fl->lock);
	return total;
}
u32 total_phy_avail(){
	return total_avail(&phyflist);
}

// General interface on physical memory management
PUBLIC paddr_t alloc_phy(uint32_t pages){
    paddr_t ret = flist_alloc(&phyflist, pages) * PAGE_SIZE;
	if(ret == 0) set_errno(ENOMEM);
	return ret;
}
PUBLIC void free_phy(paddr_t addr, uint32_t pages){
	ASSERT_ARG((addr >> 48) == 0, "%p", addr);
    flist_dealloc(&phyflist, addr / PAGE_SIZE, pages);
}