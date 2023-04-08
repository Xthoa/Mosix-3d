#include "baseapi.h"
#include "handle.h"
#include "kheap.h"
#include "exec.h"

Export int readfd(int fd, char* dst, size_t sz){
    return hop_readfile(fd, dst, sz);
}
Export int writefd(int fd, char* src, size_t sz){
    return hop_writefile(fd, src, sz);
}

Export void* kmalloc(size_t sz){
    return kheap_alloc(sz);
}
Export void kfree(void* ptr){
    kheap_free(ptr);
}

void dlentry(){}
