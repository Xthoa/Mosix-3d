#pragma once

#include "types.h"
#include "spin.h"

#define DISPATCH_FIRST 1    // 0=all 1=first
#define DISPATCH_ONCE 2     // 0=nolimit 1=once

#define DISPATCH_PROCESS 1
#define DISPATCH_MUTEX 2
#define DISPATCH_TIMER 3

#pragma pack(1)
typedef struct s_Process Process;
typedef struct s_Waitlist{
    Spinlock lock;
    volatile uint8_t count;
    uint8_t type;
    uint8_t flags;
    Process** list;
} Dispatcher;   // 12
typedef struct s_Mutex{
    Dispatcher waiter;
    Process* owner;
    volatile uint8_t owned; // 20
    u8 href;    // handle reference count
} Mutex;


void acquire_mutex(Mutex* m);
void release_mutex(Mutex* m);
void init_mutex(Mutex* m);

void init_dispatcher(Dispatcher* wl, u8 type);
void wait_dispatcher(Dispatcher* d);
void wait_multiple_dispatcher(Dispatcher** list, u32 count);
