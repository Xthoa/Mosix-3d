#include "apic.h"
#include "vmem.h"
#include "cpuid.h"
#include "asm.h"
#include "msr.h"

Bool x2apic;    // shows if the machine supports x2APIC

// Raw r/w of lapic/ioapic registers
PRIVATE u32 GetLapicVar(LapicVarID varid){
	if(x2apic)
		return ReadMSR(0x800 + varid);
	else
		return *((u32*)(LAPIC_LIN + varid * 0x10));
}
PRIVATE void SetLapicVar(LapicVarID varid, u32 val){
	if(x2apic)
		WriteMSR(0x800 + varid, val);
	else
		*((u32*)(LAPIC_LIN + varid * 0x10)) = val;
}
PRIVATE void SetLapicVar64(LapicVarID varid, u64 val){
	if(x2apic)
		WriteMSR(0x800 + varid, val);
	else{
		if(val >> 32){
            *((u32*)((u64)LAPIC_LIN + (varid + 1) * 0x10)) = val >> 32;
        }
		*((u32*)((u64)LAPIC_LIN + varid * 0x10)) = val & 0xffffffff;
	}
}
PRIVATE u32 GetIoapicVar(IoapicVarID varid){
	IoRegSelect = varid;
	mfence();
	return IoData;
}
PRIVATE void SetIoapicVar(IoapicVarID varid, u32 val){
	IoRegSelect = varid;
	mfence();
	IoData = val;
	mfence();
}
PRIVATE u64 GetIoapicVar64(IoapicVarID varid){
	IoRegSelect = varid + 1;
	mfence();
	u32 hi = IoData;
	mfence();
	IoRegSelect = varid;
	mfence();
	u32 lo = IoData;
	mfence();
	return ((u64)hi << 32) + lo;
}
PRIVATE void SetIoapicVar64(IoapicVarID varid, u64 val){
	IoRegSelect = varid + 1;
	mfence();
	IoData = val >> 32;
	mfence();
	IoRegSelect = varid;
	mfence();
	IoData = val & 0xffffffff;
	mfence();
}

// Further boxing of registers r/w
PUBLIC u64 GetRedirect(uint32_t irq){
	return GetIoapicVar64(Redirtab + irq * 2);
}
PUBLIC void SetRedirect(uint32_t irq, IntrRedirect entry){
    register uint64_t val = *(u64*)&entry;
	SetIoapicVar64(Redirtab + irq * 2, val);
}
PUBLIC u32 GetLapicId(){
	u32 val = GetLapicVar(Lapicid);
	if(x2apic) return val;
	return val >> 24;
}
PUBLIC void WriteEoi(){
    SetLapicVar(EndOfInterrupt, 0);
}
PUBLIC void SendIpi(int lapicid, IpiCommand ipi){
	if(lapicid >= 0){
		if(x2apic)ipi.dest = lapicid;
		else ipi.dest = ((u32)lapicid)<<24;
	}
	else ipi.destabbr = -lapicid;
	SetLapicVar64(InterruptCommand, *(u64*)&ipi);
}

PRIVATE void disable_pic(){
	outb(0x20,0x11);
	outb(0xa0,0x11);
	outb(0x21,0x20);
	outb(0xa1,0x28);
	outb(0x21,0x04);
	outb(0xa1,0x02);
	outb(0x21,0x01);
	outb(0xa1,0x01);
	outb(0x21,0xff);
	outb(0xa1,0xff);
	outb(0x22,0x70);
	outb(0x23,0x01);
}
void ioapic_init(){
    disable_pic();

    // map the memory-io spaces
    const uint32_t mapattr = PGATTR_NOCACHE | PGATTR_WTHRU | PGATTR_NOEXEC;
    set_mapping(IOAPIC_LIN, IOAPIC_PHY, mapattr);
    set_mapping(LAPIC_LIN, LAPIC_PHY, mapattr);

    // init the redirect table
	IntrRedirect rte={
		.mask = True,
		.vector = 0x20
	};
	for(int i=0;i<24;i++){
		SetRedirect(i, rte);
		rte.vector ++;
	}
}
void mask_all_lvt(){
	LvtRegister reg={
		.mask=True
	};
	u32 val = *(u32*)&reg;
	//SetLapicVar(LvtCmci, val);
	SetLapicVar(LvtThermal, val);
	SetLapicVar(LvtPerformance, val);
	SetLapicVar(Lint0, val);
	SetLapicVar(Lint1, val);
	SetLapicVar(LvtError, val);
}
void lapic_init(){
	Cpuid cpuid;
	GetCpuid(&cpuid, 1);
	x2apic = (cpuid.ecx >> 21) & 1;

    // enable globally
	u64 msr = ReadMSR(ApicBase);
    u64 org = msr;
	if(x2apic) msr |= 0x400;
	msr |= 0x800;
    if(org != msr)  // not initialized (as desired) yet
	    WriteMSR(ApicBase, msr);

    mask_all_lvt();
	SetLapicVar(LapicErrorStatus, 0);
	SetLapicVar(LapicErrorStatus, 0);
	SetLapicVar(EndOfInterrupt, 0);
	SetLapicVar(TaskPriority, 0);

    // enable softly
	u32 svr = GetLapicVar(SpuriousIntVector);
	svr |= 0x100;
	SetLapicVar(SpuriousIntVector, svr);
}