#include "types.h"
#include "vmem.h"
#include "pmem.h"
#include "asm.h"
#include "macros.h"

PUBLIC vaddr_t make_vaddr(u16 pml4o, u16 pdpto, u16 pdo, u16 pto, u16 offset){
    vaddr_t addr = pml4o > 0xff ? 0xffff : 0;
    addr <<= 16;
    addr |= pml4o;
    addr <<= 9;
    addr |= pdpto;
    addr <<= 9;
    addr |= pdo;
    addr <<= 9;
    addr |= pto;
    addr <<= 9;
    addr |= offset;
    return addr;
}

PUBLIC void set_pte(pte_t* pte, paddr_t addr, uint32_t attr){
    *(u64*)pte = 0;
    pte->present = True;
    pte->addr = addr >> 12;
    pte->write = attr&PGATTR_READONLY ? False : True;
    pte->user = attr&PGATTR_USER ? True : False;    // due to bitwise boolean
    pte->wthru = attr&PGATTR_WTHRU ? True : False;
    pte->nocache = attr&PGATTR_NOCACHE ? True : False;
    pte->xd = attr&PGATTR_NOEXEC ? True : False;
}
PUBLIC void set_pde(pde_t* pde, paddr_t addr, uint32_t attr){
    *(u64*)pde = 0;
    pde->present = True;
    pde->addrpt = addr >> 12;
    pde->pg2m = False;
    pde->write = attr&PGATTR_READONLY ? False : True;
    pde->user = attr&PGATTR_USER ? True : False;    // due to bitwise boolean
    pde->wthru = attr&PGATTR_WTHRU ? True : False;
    pde->nocache = attr&PGATTR_NOCACHE ? True : False;
    pde->xd = attr&PGATTR_NOEXEC ? True : False;
}
PUBLIC void set_pdpte(pdpte_t* pdpte, paddr_t addr, uint32_t attr){
    *(u64*)pdpte = 0;
    pdpte->present = True;
    pdpte->addrpd = addr >> 12;
    pdpte->pg1g = False;
    pdpte->write = attr&PGATTR_READONLY ? False : True;
    pdpte->user = attr&PGATTR_USER ? True : False;    // due to bitwise boolean
    pdpte->wthru = attr&PGATTR_WTHRU ? True : False;
    pdpte->nocache = attr&PGATTR_NOCACHE ? True : False;
    pdpte->xd = attr&PGATTR_NOEXEC ? True : False;
}
PUBLIC void set_pml4e(pml4e_t* pml4e, paddr_t addr, uint32_t attr){
    *(u64*)pml4e = 0;
    pml4e->present = True;
    pml4e->addr = addr >> 12;
    pml4e->write = attr&PGATTR_READONLY ? False : True;
    pml4e->user = attr&PGATTR_USER ? True : False;    // due to bitwise boolean
    pml4e->wthru = attr&PGATTR_WTHRU ? True : False;
    pml4e->nocache = attr&PGATTR_NOCACHE ? True : False;
    pml4e->xd = attr&PGATTR_NOEXEC ? True : False;
}

PUBLIC pte_t* get_mapping_pte(vaddr_t addr){
    return SELF_REF_ADDR + get_index(addr) * 8;
}
PUBLIC pde_t* get_mapping_pde(vaddr_t addr){
    return SELF_REF2_ADDR + (get_index(addr) >> 9) * 8;
}
PUBLIC pdpte_t* get_mapping_pdpte(vaddr_t addr){
    return SELF_REF3_ADDR + (get_index(addr) >> 18) * 8;
}
PUBLIC pml4e_t* get_mapping_pml4e(vaddr_t addr){
    return SELF_REF4_ADDR + (get_index(addr) >> 27) * 8;
}

PUBLIC paddr_t get_mapped_phy(vaddr_t addr){
    return get_page_phy(*get_mapping_pte(addr));
}
PUBLIC void set_mapped_phy(vaddr_t addr, paddr_t phy){
    pte_t* pte = get_mapping_pte(addr);
    pte->addr = phy >> 12;
}

PUBLIC void set_mapping_route(vaddr_t addr, uint32_t attr){
    pml4e_t* pml4e = get_mapping_pml4e(addr);
    if(!pml4e->present){
        set_pml4e(pml4e, alloc_phy(1), PGATTR_USER);
        pdpt_t pdpt = align_4k(get_mapping_pdpte(addr));
        memset(pdpt, 0, PAGE_SIZE);
    }
    pdpte_t* pdpte = get_mapping_pdpte(addr);
    if(!pdpte->present){
        set_pdpte(pdpte, alloc_phy(1), PGATTR_USER);
        pd_t pd = align_4k(get_mapping_pde(addr));
        memset(pd, 0, PAGE_SIZE);
    }
    pde_t* pde = get_mapping_pde(addr);
    if(!pde->present){
        set_pde(pde, alloc_phy(1), PGATTR_USER);
        pt_t pt = align_4k(get_mapping_pte(addr));
        memset(pt, 0, PAGE_SIZE);
    }
}
PUBLIC void set_mapping_entry(vaddr_t addr, paddr_t phy, uint32_t attr){
    pte_t* pte = get_mapping_pte(addr);
    // !!! CAUTION !!!
    // Note: I found that on Bochs, TLB doesn't reflush, which means
    // after resetting the mapping, the access to the linear would
    // lead to the OLD physical address. (DK whether ok on real machine)
    // "invlpg addr" can invalidate TLB of specified addr.
    invlpg(addr);
    set_pte(pte, phy, attr);
}
PUBLIC void set_mapping(vaddr_t addr, paddr_t phy, uint32_t attr){
    set_mapping_route(addr, 0);
    set_mapping_entry(addr, phy, attr);
}
PUBLIC void clear_mapping_entry(vaddr_t addr){
    pte_t* pte = get_mapping_pte(addr);
    pte->present = False;
}

PUBLIC void set_mappings(vaddr_t addr, paddr_t phy, u32 size, u32 attr){
    u64 linidx = addr >> 12;
    set_mapping_route(addr, 0);
    for(int i = 0; i < size; i++, addr += PAGE_SIZE, phy += PAGE_SIZE){
        if((addr >> 12) != linidx) set_mapping_route(addr, 0);
        set_mapping_entry(addr, phy, attr);
    }
}

PUBLIC vaddr_t malloc_page4k(u32 pages){
    paddr_t phy = alloc_phy(pages);
    vaddr_t lin = kernel_p2v(phy);
    set_mappings(lin, phy, pages, 0);
    return lin;
}
PUBLIC vaddr_t malloc_page4k_attr(u32 pages, uint32_t attr){
    paddr_t phy = alloc_phy(pages);
    vaddr_t lin = kernel_p2v(phy);
    set_mappings(lin, phy, pages, attr);
    return lin;
}
PUBLIC void free_page4k(vaddr_t lin, u32 pages){
    paddr_t phy = kernel_v2p(lin);
    free_phy(phy, pages);
}
