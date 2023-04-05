#include "proc.h"
#include "exec.h"
#include "handle.h"

Process* exec_setstdfp(char* name, File* stdfp){
    Process* old = GetCurrentProcess();
    Process* new = ExecuteFileSuspend(name);
    htab_assign(&new->htab, 0, stdfp, HANDLE_FILE);
    htab_assign(&new->htab, 1, stdfp, HANDLE_FILE);
    htab_assign(&new->htab, 2, stdfp, HANDLE_FILE);
    ready_process(new);
    return new;
}
Process* exec_dupall(char* name){
    Process* old = GetCurrentProcess();
    Process* new = ExecuteFileSuspend(name);
    htab_copy(&new->htab, &old->htab);
    ready_process(new);
    return new;
}

Process* fork_process(){
    Process* old = GetCurrentProcess();
    //Process* new = create_process(old->name, );

}