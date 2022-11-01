#pragma once

#include "types.h"

#define IOAPIC_PHY (0xfec00000)
#define LAPIC_PHY (0xfee00000)
#define IOAPIC_LIN (KERNEL_BASE+0x55000)
#define LAPIC_LIN (KERNEL_BASE+0x56000)

#define IoRegSelect (*(u8*)IOAPIC_LIN)
#define IoData (*(u32*)(IOAPIC_LIN + 0x10))
#define IoEOI (*(u32*)(IOAPIC_LIN + 0x40))

// Variable ID of Local APIC and I/O APIC
typedef enum e_LapicVarId{
	Lapicid = 0x2,
	LapicVersion,
	TaskPriority = 0x8,
	ArbiPriority,
	ProcPriority,
	EndOfInterrupt,
	SpuriousIntVector = 0xf,	// initial value 0xff
	LapicInService,
	TriggerMode = 0x18,
	InterruptRequest = 0x20,
	LapicErrorStatus = 0x28,   // Must write 0 before read
	LvtCmci = 0x2f,
	InterruptCommand,
	LvtTimer = 0x32,
	LvtThermal,	//Thermal monitor
	LvtPerformance,
	Lint0,
	Lint1,
	LvtError,
	TimerInit,
	TimerCurrent,
	TimerDivConf = 0x3e
} LapicVarID;

typedef enum IoapicVarID{
	IoapicID=0,
	IoapicVer=1,
	Redirtab=0x10	// 24 entries (irqs) in all
} IoapicVarID;

// Destination abbrivation of processor
enum e_DestAbbr{
	AbbrSelf=1,
	AbbrAll=2,
	AbbrOthers=3
};

// Delivery mode for different use
enum e_DeliveryMode{
	Fixed = 0,
	LowPriority,
	SystemManagement,
	NonMaskable = 4,
	IpiInit,
	IpiStartup,
	ExtIntr
};

// IPI command register for lapic
typedef struct s_IpiCommand{
	u8 vector;
	u8 delivmod:3;
	u8 destmod:1;	//0:phy 1:logic
	u8 delivstat:1;
	u8 :1;
	u8 drvlev:1;	//must be 1
	u8 trigmod:1;	//0:edge 1:level
	u8 :2;
	u8 destabbr:2;
	u64 :12;
	u32 dest;
} tight IpiCommand;

// Local int source
typedef struct s_LvtRegister{
	u8 vector;
	u8 delivmod:3;
	u8 :1;
	u8 delivstat:1 RO;	//0:idle 1:send pending
	u8 trigpol:1;	//0:high 1:low
	u8 remirr:1 RO;
	u8 trigmod:1;	//0:edge 1:level
	u8 mask:1;
	u8 timermode:2;	//0:1-shot 1:periodic 2:tsc-deadline
	u16 :13;
} tight LvtRegister;

// Redirection table entry for ioapic
typedef struct s_IntrRedirect{
	u8 vector;
	u8 delivmod:3;
	u8 destmod:1;	//0:phy 1:logic
	u8 delivstat:1;
	u8 trigpol:1;	//0:high 1:low
	u8 remirr:1 RO;
	u8 trigmod:1;	//0:edge 1:level
	u8 mask:1;
	u64 :15;
	u32 dest;
} tight IntrRedirect;

u64 GetRedirect(uint32_t irq);
void SetRedirect(uint32_t irq, IntrRedirect entry);
u32 GetLapicId();
void WriteEoi();
void SendIpi(int lapicid, IpiCommand ipi);