#include "exec.h"
#include "msglist.h"
#include "types.h"
#include "proc.h"

MessageList* sysprocml;

// this process is used to finish workitems or do tests
void sysproc(){
    sysprocml = create_msglist();

    Process* p = ExecuteFile("/files/boot/init.exe");
    
    while(1){
        Message msg;
        recv_message(sysprocml, &msg);
        printk("recv %w %w %d %q\n", msg.type, msg.id, msg.arg32, msg.arg64);
    }
}
