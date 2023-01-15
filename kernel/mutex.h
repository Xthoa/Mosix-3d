#pragma once

#include "types.h"
#include "spin.h"

typedef struct s_Process Process;
typedef struct s_Waitlist{
    Spinlock lock;
    volatile uint16_t count;
    Process** list;
} Waitlist;
typedef struct s_Mutex{
    volatile uint8_t owned;
    Waitlist waiter;
} Mutex;

void acquire_mutex(Mutex* m);
void release_mutex(Mutex* m);
void init_mutex(Mutex* m);
void init_waitlist(Waitlist* wl);