#include "types.h"

void entry(){
    puts("Hello from test.dll\n");
}
Export void testfunc(){
    puts("Hello from test.dll!testfunc\n");
}