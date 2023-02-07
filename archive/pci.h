#pragma once

#include "types.h"
#include "spin.h"

typedef struct s_PciBase{
	u8 ioport:1;	//0=memio 1=portio
	u8 size1m:1;	//1=larger than 1M
	u8 addr64:1;	//1=use 64bit address
	u8 prefetch:1;
	u32 addr:28;
} tight PciBase;
typedef struct s_PciAddr{
	u8 :2;
	u8 reg:6;
	u8 func:3;
	u8 dev:5;
	u8 bus:8;
	u8 :7;
	u8 enable:1;
} PciAddr;
typedef struct s_PciHead{
	union{
		struct{
			u16 vendor;
			u16 device;
			u16 command;
			u16 status;
			u8 revision;
			u8 progif;
			u8 subclass;
			u8 baseclass;
			u8 cache;
			u8 timer;
			u8 headtype:6;
			u8 multifunc:1;
			u8 :1;
			u8 bist;
			union{
				PciBase base[6];
				u32 baseval[6];
			};
			u32 cisptr;
			u16 subvendor;
			u16 subdevice;
			u32 xrombase;
			u8 capptr;
			u8 pad[7];
			u8 irqline;
			u8 irqpin;
			u8 mingnt;
			u8 maxlat;
		};
		u32 regs[16];
	};
} PciHead;
typedef struct s_PciDevice{
    struct s_PciDevice* prev;
    struct s_PciDevice* next;
	PciAddr addr;
	PciHead head;
} PciDevice;

typedef struct s_MsiCap{
	u8 id;	// = 0x05
	u8 next;
	union{
		u16 msgctrl;
		struct{
			u8 enable:1;
			u8 multiable:3;
			u8 multied:3;
			u8 bits64:1;
			u8 prevecmask:1;
			u8 :7;
		};
	};
	u64 msgaddr;
	u16 msgdata;
	u16 rsvd;
	u32 mask;
	u32 pending;
} MsiCap;
