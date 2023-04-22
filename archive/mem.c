#include "types.h"
#include "ttyio.h"
#include "pmem.h"
#include "kheap.h"
#include "exec.h"

const char* helpmsg = "mem: show memory usage\noptions:\n  -k  show kheap\n\
  -K  disable -k\n  -p  show physical memory\n  -P  disable -p\n  -h  show help\n";
void help(){
    tty_puts(helpmsg);
}
void entry(){
    Bool phy = True;
    Bool kheap = False;
    char* argv = getargv();
    if(argv){
        int len = kstrlen(argv);
        for(int i = 0; i < len; i++){
            if(argv[i] == '-'){
                if((++i) >= len) {
                    tty_puts("unknown option: '-'\n");
                    return;
                }
                char opt = argv[i];
                if(opt == 'k') kheap = True;
                elif(opt == 'K') kheap = False;
                elif(opt == 'p') phy = True;
                elif(opt == 'P') phy = False;
                elif(opt == 'h') {
                    help();
                    return;
                }
                else {
                    tty_printf("unknown option: '-%c'\n", opt);
                    return;
                }
            }
        }
    }
    u32 a, t, u;
    if(phy){
        a = memtotal + 256;
        t = total_phy_avail();
        u = a - t;
        tty_printf("Memory info:\nTotal  %4d M %4d K\nUsed   %4d M %4d K\nFree   %4d M %4d K\n",
            a >> 8, (a*4) % 1024, u >> 8, (u*4) % 1024, t >> 8, (t*4) % 1024);
    }
    if(kheap){
        a = KHEAP_INITIAL_SIZE;
        t = total_avail(&kheapflist);
        u = a - t;
        tty_printf("Kernel heap info:\nTotal  %4d K %4d B\nUsed   %4d K %4d B\nFree   %4d K %4d B\n",
            a >> 10, a % 1024, u >> 10, u % 1024, t >> 10, t % 1024);
    }
}
