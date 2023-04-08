#include "string.h"
#include "ttyio.h"
#include "asm.h"
#include "kheap.h"
#include "exec.h"
#include "proc.h"

Node* cwd;

const char* help_prompt = "Shell0 for Mosix 3d (sh0.exe)\n\
exit - exit shell\n\
help - show this help message\n\
ver - show version of system\n";

int parsecmd(char* line){
    if(!strncmp(line, "cd ", 3)){
        char* arg = line + 3;
        if(!strncmp(arg, "..", 2)) cwd = cwd->father;
        elif(arg[0] == '.') cwd = cwd;
        else cwd = find_node_from(cwd, arg);
    }
    elif(!strncmp(line, "pwd", 3)){
        for(Node* n = cwd; n != n->father; n = n->father){
            tty_printf("%s/", cwd->name);
        }
        tty_putchar('\n');
    }
    elif(!strncmp(line, "exit", 4)) return 1;
    elif(!strncmp(line, "ver", 3)){
        tty_puts("Mosix 3d Version 18\n");
    }
    elif(!strncmp(line, "help", 4)){
        tty_puts(help_prompt);
    }
    else {
        Node* n = path_walk(line);
        if(!n) n = find_node_from(cwd, line);
        if(n){
            Process* p = exec_dupstdfp(line);
            wait_process(p);
        }
        else tty_puts("Unknown command\n");
    }
    return 0;
}

void entry(){
    cwd = path_walk("/");
    
    tty_puts("Shell0 for Mosix 3d\n");
    char* line = kheap_alloc(64);
    while(True){
        tty_putchar('>');
        int len = tty_gets(line, 63);
        line[len - 1] = '\0';
        int r = parsecmd(line);
        if(r == 1) break;
    }
}