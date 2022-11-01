#include "proc.h"

// Due to 'fork' is extra complicated:
// we implement fork in an independent file.

// Fork also has sth to do with the image-segments
// so there's lots of flags to control its action

PUBLIC int fork_process(){
    Process* old = GetCurrentProcess();
    Process* new = create_process(old->name);

}