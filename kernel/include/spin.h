#pragma once

#include "types.h"

#pragma pack(1)

typedef struct s_Spinlock{
	volatile uint8_t owned;
} Spinlock;



void acquire_spin(Spinlock* l);
void release_spin(Spinlock* l);
void init_spinlock(Spinlock* l);
// init_spinlock is completely same
// as release_spin in code