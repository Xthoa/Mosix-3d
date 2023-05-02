#include "proc.h"
#include "exec.h"
#include "handle.h"
#include "kheap.h"

const char* getargv(){
    return GetCurrentProcess()->argv;
}

void fork_setstdfp(Process* new, File* stdfp){
    if(!new) return NULL;
    htab_assign(&new->htab, 0, stdfp, HANDLE_FILE);
    htab_assign(&new->htab, 1, stdfp, HANDLE_FILE);
    htab_assign(&new->htab, 2, stdfp, HANDLE_FILE);
}
void fork_dupall(Process* new, Process* old){
    if(!new) return NULL;
    htab_copy(&new->htab, &old->htab);
}
void fork_dupstdfp(Process* new, Process* old){
    if(!new) return NULL;
    memcpy(new->htab.table, old->htab.table, sizeof(Handle) * 3);
}
void fork_copycwd(Process* new, Process* old){
    if(!new) return NULL;
    new->cwd = old->cwd;
}
void fork_setargv(Process* new, char* argv){
    if(!new) return NULL;
    new->argv = kheap_clonestr(argv);
}

Process* exec_setstdfp(char* name, File* stdfp){
    Process* old = GetCurrentProcess();
    Process* new = ExecuteFileSuspend(name);
    if(!new) return NULL;
    fork_setstdfp(new, stdfp);
    fork_copycwd(new, old);
    ready_process(new);
    return new;
}
Process* exec_dupall(char* name){
    Process* old = GetCurrentProcess();
    Process* new = ExecuteFileSuspend(name);
    if(!new) return NULL;
    fork_dupall(new, old);
    fork_copycwd(new, old);
    ready_process(new);
    return new;
}
Process* exec_dupstdfp(char* name){
    Process* old = GetCurrentProcess();
    Process* new = ExecuteFileSuspend(name);
    if(!new) return NULL;
    fork_dupstdfp(new, old);
    fork_copycwd(new, old);
    ready_process(new);
    return new;
}
Process* exec_copycwd(char* name){
    Process* old = GetCurrentProcess();
    Process* new = ExecuteFileSuspend(name);
    if(!new) return NULL;
    fork_copycwd(new, old);
    ready_process(new);
    return new;
}

Process* fork_process(char* name, int type, void* arg){
    return NULL;
}