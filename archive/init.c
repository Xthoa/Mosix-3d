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

    LoadDriver("/files/boot/pci.drv");
    LoadDriver("/files/boot/ps2kbd.drv");
    LoadDriver("/files/boot/idedisk.drv");
    LoadDriver("/files/boot/pty.drv");

    ExecuteFile("/files/boot/vgatty.exe");
}
