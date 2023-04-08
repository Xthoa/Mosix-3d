#include "types.h"
#include "ttyio.h"
#include "pmem.h"

void entry(){
    u32 a = memtotal;
    u32 t = total_phy_avail();
    u32 u = a - t;
    tty_printf("Memory info:\nTotal  %4d M %3d K\nUsed   %4d M %3d K\nFree   %4d M %3d K\n",
        a >> 8, (a*4) % 1024, u >> 8, (u*4) % 1024, t >> 8, (t*4) % 1024);
}
