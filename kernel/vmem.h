#pragma once

#include "types.h"

#define KERNEL_PML4 (KERNEL_BASE+0x50000)
#define KERNEL_PML4_PHY 0x1000

#define CANONICAL_SIGN (0xffffull << 48)
#define SELF_REF_INDEX 0x1feull
#define SELF_REF_ADDR (CANONICAL_SIGN + (SELF_REF_INDEX << 39))
#define SELF_REF2_ADDR (SELF_REF_ADDR + (SELF_REF_INDEX << 30))
#define SELF_REF3_ADDR (SELF_REF2_ADDR + (SELF_REF_INDEX << 21))
#define SELF_REF4_ADDR (SELF_REF3_ADDR + (SELF_REF_INDEX << 12))

#define get_index(a) (((u64)(a)>>12)&0xfffffffff)
#define extract_pml4o_macro(a) (((u64)(a)>>39)&0x1ff)
#define extract_pdpto_macro(a) (((u64)(a)>>30)&0x1ff)
#define extract_pdo_macro(a) (((u64)(a)>>21)&0x1ff)
#define extract_pto_macro(a) (((u64)(a)>>12)&0x1ff)

#define pages4k(s) (((u64)(s)+0xfff)>>12)
#define pages2m(s) (((u64)(s)+0x1fffff)>>21)

#define page4koff(s) ((u64)(s)&0xfff)
#define page2moff(s) ((u64)(s)&0x1fffff)

#define align_4k(a) ((u64)(a)&0xfffffffffffff000)
#define align_4k_up(a) (((u64)(a)+0xfff)&0xfffffffffffff000)
#define align_2m(a) ((u64)(a)&0xffffffffffe00000)
#define align_2m_up(a) (((u64)(a)+0x1fffff)&0xffffffffffe00000)

#define get_pdpt_phy(e) (((e).addr)<<12)
#define get_pd_phy(e) (((e).addrpd)<<12)
#define get_pg2m_phy(e) (((e).addr2m)<<21)
#define get_pt_phy(e) (((e).addrpt)<<12)
#define get_page_phy(e) (((e).addr)<<12)

#define PGATTR_USER 1
#define PGATTR_NOCACHE 2
#define PGATTR_WTHRU 4
#define PGATTR_READONLY 8
#define PGATTR_NOEXEC 16

// page table structures on x86_64
typedef struct s_PML4E{
	union{
		struct{
			u8 present	:1;
			u8 write	:1;
			u8 user		:1;
			u8 wthru	:1;
			u8 nocache	:1;
			u8 accessed	:1;
			u8			:6;
			u64 addr	:40;
			u16			:11;
			u16 xd		:1;
		} tight;
		u64 entry;
	} __attribute__((gcc_union,packed));
} tight PML4E, pml4e_t, *pml4t_t, *pml4_t;
typedef struct s_PDPTE{
	union{
		struct{
			u8 present	:1;
			u8 write	:1;
			u8 user		:1;
			u8 wthru	:1;
			u8 nocache	:1;
			u8 accessed	:1;
			u8 written	:1	Optional;
			u8 pg1g		:1;
			u8 global	:1	Optional;
			u8			:3;
			u64 addrpd	:40;
				u16	pairs	:9;
				u8			:2;
			u16 xd		:1;
		}tight;
		u64 entry;
	}__attribute__((gcc_union,packed));
} tight PDPTE, pdpte_t, *pdpt_t;
typedef struct s_PDE{
	union{
		struct{
			u8 present	:1;
			u8 write	:1;
			u8 user		:1;
			u8 wthru	:1;
			u8 nocache	:1;
			u8 accessed	:1;
			u8 written	:1	Optional;
			u8 pg2m		:1;
			union{
				struct{
					u8 global	:1	Optional;
					u8			:3;
					u8 pat		:1;
					u8			:8;
					u32 addr2m	:31;
						u16	pairs	:9;
						u8			:1;
					u16 swap	:1;
					u16 xd		:1;
				}tight;
				struct{
					u8			:4;
					u64 addrpt	:40;
					u16			:12;
				}tight;
			}__attribute__((gcc_union,packed));
		}tight;
		u64 entry;
	}__attribute__((gcc_union,packed));
}tight PDE, pde_t, *pdt_t, *pd_t;
typedef struct s_PTE{
	union{
		struct{
			u8 present	:1;
			u8 write	:1;
			u8 user		:1;
			u8 wthru	:1;
			u8 nocache	:1;
			u8 accessed	:1;
			u8 written	:1;
			u8 pat		:1;
			u8 global	:1;
			u8			:3;
			u64 addr	:40;
			u16 		:10;
			u16 swap	:1;
			u16 xd		:1;
		}tight;
		u64 entry;
	}__attribute__((gcc_union,packed));
}tight PTE, pte_t, *pt_t;

pte_t* get_mapping_pte(vaddr_t addr);
pde_t* get_mapping_pde(vaddr_t addr);
pdpte_t* get_mapping_pdpte(vaddr_t addr);
pml4e_t* get_mapping_pml4e(vaddr_t addr);

void set_pte(pte_t* pte, paddr_t addr, uint32_t attr);
void set_pde(pde_t* pde, paddr_t addr, uint32_t attr);
void set_pdpte(pdpte_t* pdpte, paddr_t addr, uint32_t attr);
void set_pml4e(pml4e_t* pml4e, paddr_t addr, uint32_t attr);

paddr_t get_mapped_phy(vaddr_t addr);
void set_mapped_phy(vaddr_t addr, paddr_t phy);
void set_mapping_route(vaddr_t addr, uint32_t attr);
void set_mapping_entry(vaddr_t addr, paddr_t phy, uint32_t attr);
void set_mapping(vaddr_t addr, paddr_t phy, uint32_t attr);
void set_mappings(vaddr_t addr, paddr_t phy, u32 size, u32 attr);
void clear_mapping_entry(vaddr_t addr);

vaddr_t malloc_page4k(u32 pages);
vaddr_t malloc_page4k_attr(u32 pages, uint32_t attr);
void free_page4k(vaddr_t lin, u32 pages);