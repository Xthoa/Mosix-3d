#include "mutex.h"
#include "kheap.h"
#include "string.h"
#include "proc.h"
#include "asm.h"
#include "macros.h"
#include "msglist.h"

PUBLIC void block_of_mutex(Mutex* m){
    u32 tmp = m->waiter.count++;
    m->waiter.list[tmp] = GetCurrentProcess();
    release_spin(&m->waiter.lock);
    suspend_process();
}
PUBLIC void release_mutex(Mutex* m){
    m->owner = NULL;
    m->owned = 1;
    vasm("pause");
    acquire_spin(&m->waiter.lock);
    if(m->waiter.count){
        Process* target = m->waiter.list[0];
        pull_back_array(m->waiter.list, m->waiter.count, 0, Process*);
        m->waiter.count--;
        ready_process(target);
    }
    release_spin(&m->waiter.lock);
}
PUBLIC void release_mutex_wakeall(Mutex* m){
    m->owner = NULL;
    m->owned = 1;
    vasm("pause");
    acquire_spin(&m->waiter.lock);
    for(int i = 0; i < m->waiter.count; i++){
        Process* target = m->waiter.list[i];
        pull_back_array(m->waiter.list, m->waiter.count, 0, Process*);
        ready_process(target);
    }
    m->waiter.count = 0;
    release_spin(&m->waiter.lock);
}
PUBLIC void init_dispatcher(Dispatcher* wl, u8 type){
    wl->type = type;
    if(type == DISPATCH_TIMER){
        wl->waiter = GetCurrentProcess();
    }
    else{
        wl->count = 0;
        wl->list = kheap_alloc(sizeof(Process*) * 16);
        init_spinlock(&wl->lock);
    }
}
void free_dispatcher(Dispatcher* wl){
    if(wl->type == DISPATCH_TIMER) return;
    kheap_free(wl->list);
}
PUBLIC void init_mutex(Mutex* m){
    m->owner = NULL;
    m->href = 0;
    init_dispatcher(&m->waiter, DISPATCH_MUTEX);
    m->owned = 1;
}

void wait_signal(Signal* m){
    while(m->state != 1){
        acquire_spin(&m->waiter.lock);
        u32 tmp = m->waiter.count++;
        m->waiter.list[tmp] = GetCurrentProcess();
        release_spin(&m->waiter.lock);
        suspend_process();
    }
}
void set_signal(Signal* m){
    release_mutex_wakeall(m);
}
void clear_signal(Signal* m){
    m->state = 0;
}
