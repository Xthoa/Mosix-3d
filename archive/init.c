#include "types.h"
#include "exec.h"
#include "proc.h"
#include "msglist.h"
#include "asm.h"

void entry(){
    puts("Hello, init!\n");
    LoadDriver("/files/boot/pci.drv");
    LoadDriver("/files/boot/ps2kbd.drv");
}
