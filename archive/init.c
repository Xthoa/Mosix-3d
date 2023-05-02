#include "types.h"
#include "exec.h"
#include "proc.h"
#include "msglist.h"
#include "asm.h"
#include "handle.h"
#include "except.h"
#include "kheap.h"
#include "string.h"

void entry(){
    Node* n = path_walk("/run").node;
    n = create_subdir(n, "dev", 0);
    chdir("/files/boot");

    LoadDriver("pci.drv");
    LoadDriver("ps2kbd.drv");
    LoadDriver("idedisk.drv");
    LoadDriver("mbr.drv");
    LoadDriver("pty.drv");

    exec_copycwd("vgatty.exe");
}
