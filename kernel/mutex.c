#include "mutex.h"
#include "kheap.h"
#include "string.h"
#include "proc.h"
#include "asm.h"
#include "macros.h"

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
PUBLIC void init_dispatcher(Dispatcher* wl, u8 type){
    wl->count = 0;
    wl->list = kheap_alloc(sizeof(Process*) * 16);
    wl->type = type;
    init_spinlock(&wl->lock);
}
PUBLIC void init_mutex(Mutex* m){
    m->owner = NULL;
    m->href = 0;
    init_dispatcher(&m->waiter, DISPATCH_MUTEX);
    m->owned = 1;
}

void wait_dispatcher(Dispatcher* d){
    Process* p = GetCurrentProcess();
    p->waitcnt = 1;
    p->waitings = (Dispatcher**)d;
    if(d->type == DISPATCH_MUTEX) acquire_mutex((Mutex*) d);
    elif(d->type == DISPATCH_PROCESS) wait_process((Process*) d);
    p->waitcnt = 0;
    p->waitings = NULL;
}
void wait_multiple_dispatcher(Dispatcher** list, u32 count){
    Process* p = GetCurrentProcess();
    /*p->waitcnt = count;
    p->waitings = list;*/
}