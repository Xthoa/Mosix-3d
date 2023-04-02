#include "types.h"
#include "exec.h"
#include "proc.h"
#include "msglist.h"
#include "asm.h"
#include "handle.h"
#include "except.h"
#include "kheap.h"

void entry(){
    Node* n = path_walk("/run");
    n = create_subnode(n, "dev", 0);

    LoadDriver("/files/boot/pci.drv");
    LoadDriver("/files/boot/ps2kbd.drv");
    LoadDriver("/files/boot/idedisk.drv");

    char* name = "/run/dev/ide0";
    for(int i = 0; i < 4; i++, name[12]++){
        Node* f = path_walk(name);
        if(!f) continue;
        int fd = hop_openfile(name, 0);
        char* buf = kheap_alloc(513);
        hop_readfile(fd, buf, 1);
        buf[512] = 0;
        hop_closefile(fd);
        printk("dump of %s\n", name);
        dump_mem(buf, 128);
        kheap_free(buf);
        break;
    }

    int fd = hop_openfile("/run/dev/kbd", 0);
    while(True){
        int c;
        hop_readfile(fd, &c, 1);
        putc(c);
    }
}
