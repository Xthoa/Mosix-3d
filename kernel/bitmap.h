#pragma once

#include "types.h"
#include "spin.h"

typedef struct s_Bitmap{
	u32 max;
	u32 size;
    Spinlock lock;
	char data[];
} Bitmap;

Bitmap* create_bitmap(u32 size);
void destroy_bitmap(Bitmap* map);

off_t alloc_bit(Bitmap* map);
void free_bit(Bitmap* map,off_t bit);
