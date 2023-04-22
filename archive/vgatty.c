#include "vfs.h"
#include "cga.h"
#include "handle.h"
#include "pty.h"
#include "exec.h"
#include "kheap.h"
#include "asm.h"
#include "macros.h"
#include "proc.h"

void entry(){
    File* ptmx = open("/run/dev/ptmx", 0);
    Pty* pty = ptmx->data;
    Node* slave = pty->slave;
    File* pts = open_node(slave, 0);

    File* kbdf = open("/run/dev/kbd", 0);

    Process* sh0 = exec_setstdfp("sh0.exe", pts);

    Signal** list = kheap_alloc(sizeof(Signal*) * 3);
    list[0] = pty->out.readers;
    list[1] = ((FifoBuffer*) kbdf->data)->readers;
    list[2] = sh0->deathsig;
    
    while(True){
        int res = wait_signals(list, 3);
        switch(res){
            case 0: {
                while(!buffer_empty(&pty->out)){
                    char c = lock_read_buffer(&pty->out);
                    putc(c);
                }
                break;
            }
            case 1: {
                while(!buffer_empty((FifoBuffer*)(kbdf->data))){
                    int c = lock_read_buffer32(kbdf->data);
                    wait_write_buffer(&pty->in, c);
                }
                break;
            }
            case 2: {
                sh0 = exec_setstdfp("sh0.exe", pts);
                list[2] = sh0->deathsig;
                break;
            }
            default: break;
        }
    }
}
