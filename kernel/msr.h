#pragma once

#include "types.h"
#include "asm.h"

enum MsrAddress{
	TimeStamp=0x10,
	ApicBase=0x1b,
	SysenterCs=0x174,
	SysenterEsp=0x175,
	SysenterEip=0x176,
	Efer = 0xc0000080,
	Star,	//syscall target
	Star64,	//syscall target 64
	Lstar = 0xc0000082,
	StarCompat,
	SyscallFlagMask,
	Fsbase = 0xc0000100,
	Gsbase,
	KernGsbase
};

static inline u64 ReadMSR(u32 addr){
	u32 hi, lo;
	u64 ret;
	vasm("rdmsr":"=a"(lo),"=d"(hi):"c"(addr));
	ret=((u64)hi << 32) + lo;
	return ret;
}
static inline void WriteMSR(u32 addr,u64 val){
	u32 hi = val>>32, lo = val&0xffffffff;
	vasm("wrmsr"::"a"(lo),"d"(hi),"c"(addr));
	return;
}