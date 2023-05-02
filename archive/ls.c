#include "vfs.h"
#include "ttyio.h"
#include "proc.h"
#include "heap.h"
#include "kheap.h"
#include "asm.h"

void entry(){
    create_heap(1);
    char** buf = heap_alloc(256);
    char* dir = ".";
    char* argv = getargv();
    if(argv) dir = argv;
    File* f = open(dir, OPEN_DIR);
    if(!f){
        tty_puts("Unable to open directory\n");
        return;
    }
    while(True){
        int n = getdents(f, buf, 32);
        if(n == -1){
            tty_puts("Unable to iterate directory\n");
            break;
        }
        for(int i = 0; i < n; i++){
            tty_printf("%s\n", buf[i]);
            kheap_freestr(buf[i]);
        }
        if(n < 32) break;
    }
    close(f);
}