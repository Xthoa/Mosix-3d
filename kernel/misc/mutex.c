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
        m->waiter.list[i] = NULL;
        ready_process(target);
    }
    m->waiter.count = 0;
    release_spin(&m->waiter.lock);
}
PUBLIC void init_dispatcher(Dispatcher* wl){
    wl->count = 0;
    wl->list = kheap_alloc(sizeof(Process*) * 16);
    init_spinlock(&wl->lock);
}
void free_dispatcher(Dispatcher* wl){
    kheap_free(wl->list);
}
PUBLIC void init_mutex(Mutex* m){
    m->owner = NULL;
    m->href = 0;
    init_dispatcher(&m->waiter);
    m->owned = 1;
}
PUBLIC void final_mutex(Mutex* m){
    free_dispatcher(&m->waiter);
}

Mutex* create_mutex(){
    Mutex* m = kheap_alloc(sizeof(Mutex));
    init_mutex(m);
    return m;
}
void destroy_mutex(Mutex* m){
    final_mutex(m);
    kheap_free(m);
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

int wait_signals(Signal** list, size_t count){
    Process* self = GetCurrentProcess();
    while(True){
        for(int i = 0; i < count; i++){
            Signal* s = list[i];
            if(!s) return -1;
            if(s->state == 1) return i;
            acquire_spin(&s->waiter.lock);
            for(int j = 0; j < s->waiter.count; j++){
                Process* p = s->waiter.list[j];
                if(p == self) goto exist;
            }
            u32 tmp = s->waiter.count++;
            s->waiter.list[tmp] = self;
            exist:
            release_spin(&s->waiter.lock);
        }
        suspend_process();
    }
}
