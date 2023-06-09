#include "string.h"
#include "ttyio.h"
#include "asm.h"
#include "heap.h"
#include "exec.h"
#include "proc.h"
#include "vfs.h"

const char* help_prompt = "Shell0 for Mosix 3d (sh0.exe)\n\
exit - exit shell\n\
help - show this help message\n\
ver - show version of system\n";

Process* self;

Process* execute_new(char* line){
    int i = 0;
    while(line[i] != ' ' && line[i] != '\0') i++;
    char* argv;
    if(line[i] == '\0') argv = NULL;
    else {
        line[i] = '\0';
        argv = line + i + 1;
    }
    Process* p = ExecuteFileSuspend(line);
    if(!p) return NULL;
    fork_copycwd(p, self);
    fork_dupstdfp(p, self);
    if(argv) fork_setargv(p, argv);
    ready_process(p);
    return p;
}

int parsecmd(char* line){
    if(!strncmp(line, "cd ", 3)){
        char* arg = line + 3;
        int r = chdir(arg);
        if(r == -ENOENT){
            tty_printf("cd: %s: Not found\n", arg);
            return -1;
        }
        elif(r == -ENOTDIR){
            tty_printf("cd: %s: Not a directory\n", arg);
            return -1;
        }
    }
    elif(!strncmp(line, "exit", 4)) return 1;
    elif(!strncmp(line, "ver", 3)){
        tty_puts("Mosix 3d Version 23\n");
    }
    elif(!strncmp(line, "help", 4)){
        tty_puts(help_prompt);
    }
    else{
        Process* c = execute_new(line);
        if(!c){
            int r = get_errno();
            if(r == ENOENT) tty_printf("%s: Command not found\n", line);
            elif(r == ENOEXEC) tty_printf("%s: Exec format error\n", line);
            elif(r == EISDIR) tty_printf("%s: Is a directory\n", line);
            else tty_printf("%s: Error (%d)\n", line, r);
            return -1;
        }
        wait_process(c);
    }
    return 0;
}

void entry(){
    tty_puts("Shell0 for Mosix 3d\n");
    self = GetCurrentProcess();
    create_heap(1);
    char* line = heap_alloc(64);
    while(True){
        tty_putchar('>');
        int len = tty_gets(line, 63);
        if(len <= 1) continue;
        line[len - 1] = '\0';
        int r = parsecmd(line);
        if(r == 1) break;
    }
}