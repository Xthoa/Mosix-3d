#pragma once

#include "types.h"
#include "spin.h"

#define PmemExtentRoot (KERNEL_BASE+0x53000)

typedef struct s_Extent{
    u64 pos;
    u64 size;
} Extent;
typedef struct s_Freelist{
    Extent* root;
    u32 size;
    u32 max;
    Spinlock lock;
} Freelist;

u64 flist_alloc(Freelist *aloc,u32 size);
void flist_dealloc(Freelist* fl,u64 addr, u32 size);

u32 total_avail(Freelist* fl);
u32 total_phy_avail();

paddr_t alloc_phy(uint32_t pages);
void free_phy(paddr_t addr, uint32_t pages);

enum e_ArdsType {
    ArdsAvailable = 1,
    ArdsReserved,
    ArdsACPI,
    ArdsACPINVS,
    ArdsUnusable,
};
typedef struct s_ArdsBlock{
    u64 base;
	u64 len;
    u32 type;
} ArdsBlock;