#pragma once

#include "types.h"

#define KHEAP_EXTENT_ROOT (KERNEL_BASE+0x53800)
#define KHEAP_BASE (KERNEL_BASE+0x200000)
#define KHEAP_INITIAL_SIZE 0x1c000
#define KHEAP_MAX_SIZE 0x200000

void* kheap_alloc(u32 size);
void* kheap_alloc_zero(u32 size);
void kheap_free(void* ptr);

void* kheap_clonestr(char* str);
void kheap_freestr(char* str);
uint16_t kstrlen(char* kstr);

u32 total_kheap_avail();

#include "pmem.h"

extern Freelist kheapflist;
