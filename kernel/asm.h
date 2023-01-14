#pragma once

#include "types.h"

#define asm __asm__
#define vasm __asm__ __volatile__
#define bochsdbg() vasm("xchg %bx,%bx")
#define hlt() vasm("hlt")
#define sti() vasm("sti")
#define cli() vasm("cli")
#define mfence() vasm("mfence")
#define invlpg(addr) vasm("invlpg (%0)"::"r"(addr):"memory")
#define AsmRebootSystem() outb(0x64,0xfe)

#define lock_inc(x) asm("lock; incl %0":"+m"(x));
#define lock_dec(x) asm("lock; decl %0":"+m"(x));

static inline u64 getcr3(){
  u64 cr3;
  asm("mov %%cr3,%0":"=r"(cr3));
  return cr3;
}
static inline u64 getcr2(){
  u64 cr2;
  asm("mov %%cr2,%0":"=r"(cr2));
  return cr2;
}

static inline u64 SaveFlagsCli(){
	u64 rflags;
	vasm("pushfq;popq %0;cli":"=g"(rflags));
	return rflags;
}
static inline void LoadFlags(u64 rflags){
	vasm("pushq %0;popfq"::"g"(rflags));
}
static inline _Bool CheckIntState(){
  u64 rflags;
  vasm("pushfq;popq %0;":"=g"(rflags));
  return (rflags>>9)&1;
}

//Codes below are from xv6.
static inline void outb(u16 port, u8 data){
  vasm("out %0,%1" : : "a" (data), "d" (port));
}
static inline void outw(u16 port, u16 data){
  vasm("out %0,%1" : : "a" (data), "d" (port));
}
static inline void outl(u16 port, u32 data){
  vasm("out %0,%1" : : "a" (data), "d" (port));
}
static inline u8 inb(u16 port){
  u8 data;
  vasm("in %1,%0" : "=a" (data) : "d" (port));
  return data;
}
static inline u16 inw(u16 port){
  u16 data;
  vasm("in %1,%0" : "=a" (data) : "d" (port));
  return data;
}
static inline u32 inl(u16 port){
  u32 data;
  vasm("in %1,%0" : "=a" (data) : "d" (port));
  return data;
}
static inline void insl(int port, void *addr, int cnt){
  vasm("cld; rep insl" :
		"=D" (addr), "=c" (cnt) :
		"d" (port), "0" (addr), "1" (cnt) :
		"memory", "cc");
}
static inline void outsl(int port, const void *addr, int cnt){
  vasm("cld; rep outsl" :
		"=S" (addr), "=c" (cnt) :
		"d" (port), "0" (addr), "1" (cnt) :
		"cc");
}
static inline void outsb(int port, const void *addr, int cnt){
  vasm("cld; rep outsb" :
		"=S" (addr), "=c" (cnt) :
		"d" (port), "0" (addr), "1" (cnt) :
		"cc");
}