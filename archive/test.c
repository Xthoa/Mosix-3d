#include "types.h"

void dlentry(){
    puts("Hello from test.dll\n");
}
Export void testfunc(){
    puts("Hello from test.dll!testfunc\n");
}