#include "types.h"
#include "pmem.h"
#include "boot.h"
#include "string.h"
#include "kheap.h"

PRIVATE Freelist phyflist;

// Init freelist allocator
void pmem_init(BootArguments* bargs){
    phyflist.root = PmemExtentRoot;
    phyflist.max = 112; // 0x700 / 16 = 112
    phyflist.size = 0;

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
	u64 addr=0;
	for(int i = 0; i < aloc->size; i++){
		Extent* f=&(aloc->root[i]);
		if(f->size==size){
			addr = f->pos;
			memcpy(f, f+1, (aloc->size-(f-aloc->root))*sizeof(Extent));
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
	return addr;
}
PUBLIC void flist_dealloc(Freelist *aloc,u64 addr,u32 size){
	u32 i,len;
	for(i = 0; i < aloc->size; i++){
		Extent* f=&(aloc->root[i]);
		if(f->pos >= addr+size){
			u32 len = aloc->size - i;
			memmove(f+1, f, len*sizeof(Extent));
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

	Extent* front = &(aloc->root[i-1]);
	Extent* self = &(aloc->root[i]);
	Extent* end = &(aloc->root[i+1]);
	Bool raf,rae;
	if(i == 0) raf = False;
	else raf = front->pos + front->size == addr;
	if(i == aloc->size) rae = False;
	else rae = end->pos == addr+size;

	if(raf){
		front->size += size;
		len = aloc->size-i-1;
		memmove(self, end, len*sizeof(Extent));
		end=self;
        self=front;
		i--;
        aloc->size--;
	}
	if(rae){
		self->size += end->size;
		len = aloc->size-i-2;
		memmove(end, end+1, len*sizeof(Extent));
		aloc->size--;
	}
	return;
}

PUBLIC paddr_t alloc_phy(uint32_t pages){
    return flist_alloc(&phyflist, pages) * PAGE_SIZE;
}
PUBLIC void free_phy(paddr_t addr, uint32_t pages){
    flist_dealloc(&phyflist, addr / PAGE_SIZE, pages);
}