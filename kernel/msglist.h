#pragma once

#include "types.h"
#include "mutex.h"

typedef struct s_Message{
    uint16_t type;
    uint16_t id;
    uint32_t arg32;
    uint64_t arg64;
} Message;
typedef struct s_MessageList{
    Dispatcher dwait;
    Process* waiter;
    Message* list;
    u16 cap;     // list capacity
    u16 head;    // list head position
    u16 size;    // list size
    Spinlock lock;
} MessageList;

MessageList* create_msglist();
void destroy_msglist(MessageList* ml);

void send_message(MessageList* ml, Message* msg);
void recv_message(MessageList* ml, Message* dst);
