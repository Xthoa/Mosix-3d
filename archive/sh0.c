#include "vfs.h"
#include "handle.h"
#include "string.h"

void entry(){
    char* info = "Shell0 for Mosix 3d\n";
    hop_writefile(1, info, strlen(info));
    while(True){
        hop_writefile(1, ">", 1);
        while(True){
            char c;
            hop_readfile(0, &c, 1);
            hop_writefile(1, &c, 1);
            if(c == '\n') break;
        }
    }
}