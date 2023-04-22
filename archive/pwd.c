#include "vfs.h"
#include "ttyio.h"
#include "kheap.h"

void entry(){
    create_heap(1);
    char* buf = heap_alloc(64);
    getcwd(buf, 63);
    tty_printf("%s\n", buf);
    heap_free(buf);
}