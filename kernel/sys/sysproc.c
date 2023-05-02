#include "exec.h"
#include "msglist.h"
#include "types.h"
#include "proc.h"
#include "kheap.h"
#include "asm.h"

MessageList* sysprocml;

// this process is used to finish workitems or do tests
void sysproc(){
    sysprocml = create_msglist();

    ExecuteFile("/files/boot/init.exe");
    
    while(1){
        Message msg;
        recv_message(sysprocml, &msg);

        switch(msg.type){
            case SPMSG_REAP:{
                do_reap_process((Process*)msg.arg64);
                break;
            }
            case SPMSG_CALLBACK:{
                int (*callback)(int) = msg.arg64;
                callback(msg.arg32);
                break;
            }
            default: break;
        }
    }
}
