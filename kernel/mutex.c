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
PUBLIC void init_waitlist(Waitlist* wl){
    wl->count = 0;
    wl->list = kheap_alloc(sizeof(Process*) * 16);
    init_spinlock(&wl->lock);
}
PUBLIC void init_mutex(Mutex* m){
    m->owned = 1;
    init_waitlist(&m->waiter);
}