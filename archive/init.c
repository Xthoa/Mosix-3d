#include "types.h"
#include "exec.h"
#include "proc.h"
#include "msglist.h"
#include "asm.h"
#include "handle.h"

void entry(){
    puts("Hello, init!\n");
    LoadDriver("/files/boot/pci.drv");
    LoadDriver("/files/boot/ps2kbd.drv");

    int fd = hop_openfile("/run/dev/kbd", 0);
    while(True){
        int c;
        hop_readfile(fd, &c, 1);
        putc(c);
    }
}
