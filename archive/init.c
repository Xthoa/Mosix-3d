#include "types.h"
#include "exec.h"
#include "proc.h"
#include "msglist.h"

void entry(){
    puts("Hello, init!\n");
    
    Message msg;
    msg.type = 1;
    msg.id = 1024;
    msg.arg32 = 0xabcdef;
    msg.arg64 = &msg;
    send_message(sysprocml, &msg);
}
