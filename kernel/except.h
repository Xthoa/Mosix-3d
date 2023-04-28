#pragma once

#include "types.h"

typedef struct s_jmpbuf{
	u64 rcx;
	u64 rsi;
	u64 rbp;
	u64 rbx;
	u64 rdx;
	u64 r8;
	u64 r9;
	u64 r10;
	u64 r11;
	u64 r12;
	u64 r13;
	u64 r14;
	u64 r15;
	u64 rip;
	u64 rsp;
} JumpBuffer,Jmpbuf,*jmp_buf;

PUBLIC int setjmp(jmp_buf buf);
PUBLIC void longjmp(jmp_buf buf,int ret);
void pushjb(jmp_buf b);
jmp_buf popjb();

#define __try \
	Jmpbuf __jmpbuf; \
	int __result = setjmp(&__jmpbuf); \
	if (!__result) pushjb(&__jmpbuf); \
	if (!__result) 

#define __except(x) __catch(x)
#define __catch(x) \
	x = __result; \
	if (!x) popjb(); \
	else 

#define __raise(x) __throw(x)
#define __throw(x) \
	longjmp(popjb(), x)

#define __throws(x)
