#pragma once

#include "types.h"
#include "proc.h"
#include "spin.h"

typedef struct s_Mutex{
    volatile uint8_t owned;
    Spinlock waitlock;
    volatile uint16_t waitcnt;
    // Process* owner;
    Process** waiter;
} Mutex;

void acquire_mutex(Mutex* m);
void release_mutex(Mutex* m);
void init_mutex(Mutex* m);