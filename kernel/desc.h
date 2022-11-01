#include "types.h"

#define IntHandler __attribute__((interrupt))

#define IDT ((Gate*)(KERNEL_BASE+0x51000))
#define GDT ((Descriptor*)(KERNEL_BASE+0x52000))

typedef enum e_GateType{
	LDT=0x2,
	TssGate=0x9,
	BusyTssGate=0xb,
	CallGate=0xc,
	Interrupt=0xe,
	TrapGate=0xf,
} GateType;
typedef struct s_IDT{
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
} tight Idt,Gate;
typedef struct s_GDT{
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
} tight Gdt,Descriptor;

typedef struct s_IntFrame{
	u64 rip;
	u64 cs;
	u64 rflags;
	u64 rsp;
	u64 ss;
} IntFrame;