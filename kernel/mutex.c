#include "mutex.h"
#include "kheap.h"
#include "string.h"
#include "proc.h"
#include "asm.h"

// acquire & release, work together
// to avoid 'the awkward state': Unfetchable lock
PUBLIC void block_of_mutex(Mutex* m){
    acquire_spin(&m->waitlock);
    u32 tmp = m->waitcnt++;
    m->waiter[tmp] = GetCurrentProcess();
    release_spin(&m->waitlock);
    suspend_process();
}
PUBLIC void release_mutex(Mutex* m){
    m->owned = 1;
    if(m->waitcnt == 0)vasm("pause");
    if(m->waitcnt){
        acquire_spin(&m->waitlock);
        Process* target = m->waiter[0];
        memmove(m->waiter, m->waiter+1, sizeof(Process*) * (m->waitcnt-1));
        m->waitcnt--;
        ready_process(target);
        release_spin(&m->waitlock);
    }
}
PUBLIC void init_mutex(Mutex* m){
    m->owned = 1;
    m->waiter = kheap_alloc(sizeof(Process*) * 16);
    m->waitcnt = 0;
    // m->owner = NULL;
    init_spinlock(&m->waitlock);
}