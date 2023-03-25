#include "msglist.h"
#include "kheap.h"
#include "vmem.h"
#include "spin.h"
#include "proc.h"
#include "string.h"
#include "asm.h"
#include "cga.h"

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

void init_buffer(FifoBuffer* b, u16 objsz, u16 count, Bool rsig, Bool wsig){
    b->cap = count;
    size_t sz = b->size = objsz * count;
    if(sz <= 512) b->buf = kheap_alloc(sz);
    else b->buf = malloc_page4k(pages4k(sz));
    b->read = 0;
    b->write = 0;
    init_spinlock(&b->lock);
    if(rsig){
        b->readers = kheap_alloc(sizeof(Signal));
        init_mutex(b->readers);
    }
    else b->readers = NULL;
    if(wsig){
        b->writers = kheap_alloc(sizeof(Signal));
        init_mutex(b->writers);
    }
    else b->writers = NULL;
}
void destroy_buffer(FifoBuffer* b){
    if(b->size <= 512) kheap_free(b->buf);
    else free_page4k(b->buf, pages4k(b->size));
    if(b->readers) free_dispatcher(b->readers);
    if(b->writers) free_dispatcher(b->writers);
}

void do_send_message(MessageList* ml, Message* msg){
    acquire_spin(&ml->lock);
    if(ml->size == 0) ready_process(ml->waiter);
    u16 pos = ml->head + ml->size++;
    if(pos >= ml->cap) pos -= ml->cap;
    ((Message*)ml->list)[pos] = *msg;
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
    *dst = ((Message*)ml->list)[pos];
    release_spin(&ml->lock);
}

void send_message(MessageList* ml, Message* msg){
    while(ml->size == ml->cap) wait_msglist(ml);
    do_send_message(ml, msg);
}
void recv_message(MessageList* ml, Message* dst){
    while(ml->size == 0) wait_message(ml);
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

int read_buffer(FifoBuffer* b){
    if(buffer_empty(b)) return -1;
    if(b->writers && buffer_full(b)) set_signal(b->writers);
    int data = b->buf[b->read];
    b->read ++;
    if(b->read >= b->cap) b->read = 0;
    if(b->readers && buffer_empty(b)) clear_signal(b->readers);
    return data;
}
int lock_read_buffer(FifoBuffer* b){
    acquire_spin(&b->lock);
    int ret = read_buffer(b);
    release_spin(&b->lock);
    return ret;
}
int write_buffer(FifoBuffer* b, int data){
    if(buffer_full(b)) return -1;
    if(b->readers && buffer_empty(b)) set_signal(b->readers);
    b->buf[b->write] = data;
    b->write ++;
    if(b->write >= b->cap) b->write = 0;
    if(b->writers && buffer_full(b)) clear_signal(b->writers);
    return data;
}
int lock_write_buffer(FifoBuffer* b, int data){
    acquire_spin(&b->lock);
    int ret = write_buffer(b, data);
    release_spin(&b->lock);
    return ret;
}
int overwrite_buffer(FifoBuffer* b, int data){
    if(buffer_full(b)){
        b->read ++;
        if(b->read >= b->cap) b->read = 0;
    }
    elif(b->readers && buffer_empty(b)) set_signal(b->readers);
    b->buf[b->write] = data;
    b->write ++;
    if(b->write >= b->cap) b->write = 0;
    if(b->writers && buffer_full(b)) clear_signal(b->writers);
    return data;
}
int lock_overwrite_buffer(FifoBuffer* b, int data){
    acquire_spin(&b->lock);
    int ret = overwrite_buffer(b, data);
    release_spin(&b->lock);
    return ret;
}

int read_buffer32(FifoBuffer* b){
    if(buffer_empty(b)) return -1;
    if(b->writers && buffer_full(b)) set_signal(b->writers);
    int data = ((u32*)b->buf)[b->read];
    b->read ++;
    if(b->read >= b->cap) b->read = 0;
    if(b->readers && buffer_empty(b)) clear_signal(b->readers);
    return data;
}
int lock_read_buffer32(FifoBuffer* b){
    acquire_spin(&b->lock);
    int ret = read_buffer32(b);
    release_spin(&b->lock);
    return ret;
}
int write_buffer32(FifoBuffer* b, int data){
    if(buffer_full(b)) return -1;
    if(b->readers && buffer_empty(b)) set_signal(b->readers);
    ((u32*)b->buf)[b->write] = data;
    b->write ++;
    if(b->write >= b->cap) b->write = 0;
    if(b->writers && buffer_full(b)) clear_signal(b->writers);
    return data;
}
int lock_write_buffer32(FifoBuffer* b, int data){
    acquire_spin(&b->lock);
    int ret = write_buffer32(b, data);
    release_spin(&b->lock);
    return ret;
}
int overwrite_buffer32(FifoBuffer* b, int data){
    if(buffer_full(b)){
        b->read ++;
        if(b->read >= b->cap) b->read = 0;
    }
    elif(b->readers && buffer_empty(b)) set_signal(b->readers);
    ((u32*)b->buf)[b->write] = data;
    b->write ++;
    if(b->write >= b->cap) b->write = 0;
    if(b->writers && buffer_full(b)) clear_signal(b->writers);
    return data;
}
int lock_overwrite_buffer32(FifoBuffer* b, int data){
    acquire_spin(&b->lock);
    int ret = overwrite_buffer32(b, data);
    release_spin(&b->lock);
    return ret;
}

int wait_read_buffer(FifoBuffer* b){
    if(buffer_empty(b)) wait_signal(b->readers);
    return lock_read_buffer(b);
}
int wait_read_buffer32(FifoBuffer* b){
    if(buffer_empty(b)) wait_signal(b->readers);
    return lock_read_buffer32(b);
}
int wait_write_buffer(FifoBuffer* b, int data){
    if(buffer_full(b)) wait_signal(b->writers);
    return lock_write_buffer(b, data);
}
int wait_write_buffer32(FifoBuffer* b, int data){
    if(buffer_full(b)) wait_signal(b->writers);
    return lock_write_buffer32(b, data);
}
