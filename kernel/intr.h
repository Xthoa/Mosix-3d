#pragma once

#include "types.h"

#define IntHandler __attribute__((interrupt))
#define IDT (KERNEL_BASE+0x51000)
#define GDT (KERNEL_BASE+0x52000)

enum e_GateType{
	Ldt=0x2,
	TssGate=0x9,
	BusyTssGate=0xb,
	CallGate=0xc,
	Interrupt=0xe,
	TrapGate=0xf,
};

#pragma pack(1)
typedef struct s_Gate{
	Word off;
	Word sel;
	Byte ist:3;
	Byte :5;
	Byte type:4;
	Byte sys:1;
	Byte dpl:2;
	Byte present:1;
	Word off2;
	Dword off3;
	Dword pad;
} tight Gate;
typedef struct s_Segment{
	Word limit;
	Word base;
	Byte base2;
		u8 access:1;
		u8 rw:1;
		u8 direction:1;
		u8 exec:1;
		u8 gdt:1;
		u8 dpl:2;
		u8 present:1;
	u8 limit2:4;
		u8 avl:1;
		u8 size64:1;
		u8 size32:1;
		u8 pagesized:1;
	Byte base3;
} tight Segment;

typedef struct s_IntFrame{
	vaddr_t rip;
	u64 cs;
	u64 rflags;
	vaddr_t rsp;
	u64 ss;
} IntFrame;

