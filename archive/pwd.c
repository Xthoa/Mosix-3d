#include "vfs.h"
#include "ttyio.h"
#include "kheap.h"

void entry(){
    char* buf = kheap_alloc(64);
    getcwd(buf, 63);
    tty_printf("%s\n", buf);
    kheap_free(buf);
}