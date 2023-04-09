#include "string.h"
#include "ttyio.h"
#include "asm.h"
#include "kheap.h"
#include "exec.h"
#include "proc.h"

Path cwd;

const char* help_prompt = "Shell0 for Mosix 3d (sh0.exe)\n\
exit - exit shell\n\
help - show this help message\n\
ver - show version of system\n";

int parsecmd(char* line){
    if(!strncmp(line, "cd ", 3)){
        char* arg = line + 3;
        Path p = path_walk(arg);
        if(!p.node){
            p = cwd;
            find_node_from(&p, arg);
            if(!p.node){
                tty_printf("cd: %s: Not found\n", arg);
                return -1;
            }
        }
        cwd = p;
    }
    elif(!strncmp(line, "pwd", 3)){
        char* buf = kheap_alloc(64);
        path_stringify(cwd, buf);
        tty_printf("%s\n", buf);
        kheap_free(buf);
    }
    elif(!strncmp(line, "exit", 4)) return 1;
    elif(!strncmp(line, "ver", 3)){
        tty_puts("Mosix 3d Version 19\n");
    }
    elif(!strncmp(line, "help", 4)){
        tty_puts(help_prompt);
    }
    else {
        Path p = path_walk(line);
        if(!p.node){
            p = cwd;
            find_node_from(&p, line);
            if(!p.node){
                tty_puts("Unknown command\n");
                return -1;
            }
        }
        char* wp = kheap_alloc(64);
        path_stringify(p, wp);
        bochsputs(wp, strlen(wp));
        Process* c = exec_dupstdfp(wp);
        kheap_free(wp);
        wait_process(c);
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