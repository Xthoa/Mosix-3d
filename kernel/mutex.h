#pragma once

#include "types.h"
#include "spin.h"

#pragma pack(1)
typedef struct s_Process Process;
typedef struct s_Dispatcher{
    volatile uint8_t count;
    Spinlock lock;
    Process** list;
} Dispatcher;   // 12
typedef struct s_Mutex{
    union{
        volatile uint8_t owned;
        volatile uint8_t state;
    };
    Dispatcher waiter;
    Process* owner;
    u8 href;    // handle reference count
} Mutex, Signal;

void acquire_mutex(Mutex* m);
void release_mutex(Mutex* m);
void init_mutex(Mutex* m);
void final_mutex(Mutex* m);

Mutex* create_mutex();
void destroy_mutex(Mutex* m);

void wait_signal(Signal* s);
void set_signal(Signal* s);
void clear_signal(Signal* s);
int wait_signals(Signal** list, size_t count);
