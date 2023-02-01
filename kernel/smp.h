#pragma once

#include "types.h"
#include "proc.h"
#include "spin.h"

#pragma pack(1)

// ACPI Region
typedef struct s_Rsdptr{
	//1.0
	char sign[8];
	u8 chksum;
	char oemid[6];
	u8 ver;
	u32 rsdt_addr;
	//2.0
	u32 len;
	u64 xsdt_addr;
	u8 extchksum;
	u32 :24;
} tight Rsdptr;
typedef struct s_SDTHead{
	char sign[4];
	u32 len;
	u8 ver;
	u8 chksum;
	char oemid[6];
	char oemtabid[8];
	u32 oemver;
	u32 creatorid;
	u32 creatorver;
} tight SdtHead;
typedef struct s_RSDT{
	SdtHead head;
	u32 ptrs[];
} tight Rsdt;
typedef struct s_XSDT{
	SdtHead head;
	u64 ptrs[];
} tight Xsdt;

#define ACPI_PAGE0 (KERNEL_BASE+0x57000)
#define ACPI_PAGE1 (KERNEL_BASE+0x58000)

// MADT Region
struct _me_type0_lapic{
	u8 acpiprocid;	//acpi processor id
	u8 lapicid;
	u32 flags;
};	// 6
struct _me_type1_ioapic{
	u8 ioapicid;
	u8 :8;
	u32 ioapicaddr;
	u32 gsintbase;	//global sys int base
};	// 10
struct _me_type2_srcovrd{
	u8 bus;
	u8 irq;
	u32 gsint;
	u16 flags;
};	// 8
struct _me_type3_nmisrc{
	u8 nmisrc;
	u8 :8;
	u16 flags;
	u32 gsint;
};	// 8
struct _me_type4_locnmi{
	u8 acpiprocid;	//0xff=all
	u16 flags;
	u8 lint;	//0or1
};	// 4
struct _me_type5_locovrd{
	u16 :16;
	u64 lapicphy;
};	// 10
struct _me_type9_x2apic{
	u16 :16;
	u32 x2apicid;
	u32 flgs;
	u32 acpiid;
};	// 14
typedef struct s_MadtEntry{
	u8 type;
	u8 len;
	u8 data[];
} MadtEntry;
typedef struct s_Madt{
	SdtHead head;
	u32 lapicaddr;
	u32 flags;	//1=8259 exist
	MadtEntry entries[];
} Madt;

// SMP Region
typedef struct s_Tss64{
	u32 :32;
	u64 rsp0;
	u64 rsp1;
	u64 rsp2;
	u64 :64;
	u64 ist1;
	u64 ist2;
	u64 ist3;
	u64 ist4;
	u64 ist5;
	u64 ist6;
	u64 ist7;
	u64 :64;
	u16 :16;
	u16 iobase;
} Tss64; // 104
typedef struct{
	Process** list;	//+128 0x80
	u16 cnt;	    //+136 0x88
    u16 cur;
	Spinlock lock;  // +140 0x8c
} ReadyList;
typedef struct s_cpu{
	u32 lapicid;	// +0
	Tss64 tss;	    // +4
	u8 index;	    // +108 0x6c
	volatile u8 stat;
	Process* cur;	// +110 0x6e
	u16 tickleft;	// +118 0x76
	Process* idle;	// +120 0x78
	ReadyList ready;// +128 0x80
    u64 :24;
} Processor;	// 144 0x90



#define CPU_NONE 0
#define CPU_INIT 1
#define CPU_READY 2
#define CPU_GONE 3	// ipi timeout
#define CPU_ERR 4	// error while ipi

#define cpus ((Processor*)(KERNEL_BASE+0x59000))
extern u8 ncpu;

Processor* GetCurrentProcessor();
Processor* GetCurrentProcessorByLapicid();
