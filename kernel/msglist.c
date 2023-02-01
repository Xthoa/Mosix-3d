#include "msglist.h"
#include "kheap.h"
#include "vmem.h"
#include "spin.h"
#include "proc.h"

MessageList* create_msglist(){
    MessageList* ml = kheap_alloc(sizeof(MessageList));
    ml->list = malloc_page4k(1);
    ml->cap = 512;
    ml->size = 0;
    ml->head = 0;
    init_spinlock(&ml->lock);
    ml->waiter = GetCurrentProcess();
    init_dispatcher(ml, DISPATCH_MSGLIST);
    return ml;
}
void destroy_msglist(MessageList* ml){
    free_page4k(ml->list, 1);
    kheap_free(ml);
}

void do_send_message(MessageList* ml, Message* msg){
    acquire_spin(&ml->lock);
    if(ml->size == 0) ready_process(ml->waiter);
    u16 pos = ml->head + ml->size++;
    if(pos >= ml->cap) pos -= ml->cap;
    ml->list[pos] = *msg;
    release_spin(&ml->lock);
}
void do_recv_messsage(MessageList* ml, Message* dst){
    acquire_spin(&ml->lock);
    if(ml->size == ml->cap){
        acquire_spin(&ml->dwait.lock);
        for(int i = 0; i < ml->dwait.count; i++){
            ready_process(ml->dwait.list[i]);
            ml->dwait.list[i] = NULL;
        }
        ml->dwait.count = 0;
        release_spin(&ml->dwait.lock);
    }
    u16 pos = ml->head++;
    ml->size --;
    if(ml->head == ml->cap) ml->head = 0;
    *dst = ml->list[pos];
    release_spin(&ml->lock);
}

void send_message(MessageList* ml, Message* msg){
    if(ml->size == ml->cap) wait_msglist(ml);
    do_send_message(ml, msg);
}
void recv_message(MessageList* ml, Message* dst){
    if(ml->size == 0) wait_message(ml);
    do_recv_messsage(ml, dst);
}

void wait_msglist(MessageList* ml){
    acquire_spin(&ml->dwait.lock);
    u32 pos = ml->dwait.count ++;
    ml->dwait.list[pos] = GetCurrentProcess();
    release_spin(&ml->dwait.lock);
    suspend_process();
}
void wait_message(MessageList* ml){
    suspend_process();
}
