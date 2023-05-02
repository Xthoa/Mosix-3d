#pragma once

#include "types.h"
#include "mutex.h"

#define buffer_full(b) ((b)->write == (b)->read - 1 || (b)->write == ((b)->read + (b)->cap) - 1)
#define buffer_empty(b) ((b)->read == (b)->write)

typedef struct s_FifoBuffer{
    Signal* readers;
    Signal* writers;
    u8* buf;
    u16 cap;
    u16 read;
    u16 write;
    u16 size;
    Spinlock lock;
} FifoBuffer;

typedef struct s_Message{
    uint16_t type;
    uint16_t id;
    uint32_t arg32;
    uint64_t arg64;
} Message;
typedef struct s_MessageList{
    Signal* sender;
    Process* waiter;
    u8* list;
    u16 cap;     // list capacity
    u16 head;    // list head position
    u16 size;    // list size
    Spinlock lock;
} MessageList;

MessageList* create_msglist();
void destroy_msglist(MessageList* ml);

void send_message(MessageList* ml, Message* msg);
void recv_message(MessageList* ml, Message* dst);

void init_buffer(FifoBuffer* b, u16 objsz, u16 count, Bool rsig, Bool wsig);
void destroy_buffer(FifoBuffer* b);

int read_buffer(FifoBuffer* b);
int write_buffer(FifoBuffer* b, int data);
int overwrite_buffer(FifoBuffer* b, int data);
int read_buffer32(FifoBuffer* b);
int write_buffer32(FifoBuffer* b, int data);
int overwrite_buffer32(FifoBuffer* b, int data);

int lock_read_buffer(FifoBuffer* b);
int lock_write_buffer(FifoBuffer* b, int data);
int lock_overwrite_buffer32(FifoBuffer* b, int data);
int lock_read_buffer32(FifoBuffer* b);
int lock_write_buffer32(FifoBuffer* b, int data);
int lock_overwrite_buffer32(FifoBuffer* b, int data);

int wait_read_buffer(FifoBuffer* b);
int wait_read_buffer32(FifoBuffer* b);
int wait_write_buffer(FifoBuffer* b, int data);
int wait_write_buffer32(FifoBuffer* b, int data);
