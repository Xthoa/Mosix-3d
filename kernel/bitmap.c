#include "bitmap.h"
#include "kheap.h"

Bitmap* create_bitmap(u32 size){
	Bitmap* map = kheap_alloc_zero(sizeof(Bitmap) + size);
	map->max = size;
	map->size = 0;
	init_spinlock(&map->lock);
	return map;
}
void destroy_bitmap(Bitmap* map){
	kheap_free(map);
}

off_t alloc_bit(Bitmap* map){
	acquire_spin(&map->lock);
	u32 bytes=(map->max+7)>>3;
	for(int i=0;i<bytes;i++){
		u8 c = map->data[i];
		if(c == 0xff)continue;
		for(int j=0;j<8;j++){
			Bool bit=(c>>j)&1;
			if(bit==False){
				map->data[i]|=(1u<<j);
				release_spin(&map->lock);
				return i*8+j;
			}
		}
	}
	map->size++;
	release_spin(&map->lock);
	return map->max;
}
void free_bit(Bitmap* map,off_t bit){
	acquire_spin(&map->lock);
	map->data[bit>>3]&=~(1u<<(bit&3));
	map->size--;
	release_spin(&map->lock);
}
