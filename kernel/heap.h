#pragma once

#include "types.h"

#define DEFAULT_HEAPPOS 0x40000000

void create_heap(size_t rsv);
void* heap_alloc(u32 size);
void* heap_alloc_zero(u32 size);
void heap_free(void* ptr);
