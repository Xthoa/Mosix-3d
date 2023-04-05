#pragma once

#include "types.h"
#include "spin.h"

#define DISPATCH_FIRST 1    // 0=all 1=first
#define DISPATCH_ONCE 2     // 0=nolimit 1=once

#define DISPATCH_PROCESS 1
#define DISPATCH_MUTEX 2
#define DISPATCH_TIMER 3
#define DISPATCH_MSGLIST 4

#pragma pack(1)
typedef struct s_Process Process;
typedef struct s_Dispatcher{
    uint8_t type;
    Spinlock lock;
    volatile uint8_t count;
    uint8_t flags;
    union{
        Process** list;
        Process* waiter;
    };
} Dispatcher;   // 12
typedef struct s_Mutex{
    Dispatcher waiter;
    Process* owner;
    union{
        volatile uint8_t owned; // 20
        volatile uint8_t state;
    };
    u8 href;    // handle reference count
} Mutex, Signal;

void acquire_mutex(Mutex* m);
void release_mutex(Mutex* m);
void init_mutex(Mutex* m);

void wait_signal(Signal* s);
void set_signal(Signal* s);
void clear_signal(Signal* s);
int wait_signals(Signal** list, size_t count);

void init_dispatcher(Dispatcher* wl, u8 type);
void free_dispatcher(Dispatcher* wl);
void wait_dispatcher(Dispatcher* d);
void wait_multiple_dispatcher(Dispatcher** list, u32 count);
