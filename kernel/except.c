#include "except.h"
#include "proc.h"
#include "asm.h"

void pushjb(jmp_buf b){
	Process* p = GetCurrentProcess();
	p->jbstack[p->jbesp++] = b;
}
jmp_buf popjb(){
	Process* p = GetCurrentProcess();
	return p->jbstack[--p->jbesp];
}