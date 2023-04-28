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

void set_errno(u16 errno){
	GetCurrentProcess()->errno = errno;
}
u16 get_errno(){
	return GetCurrentProcess()->errno;
}
