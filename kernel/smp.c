#include "smp.h"
#include "boot.h"
#include "vmem.h"
#include "apic.h"
#include "intr.h"
#include "asm.h"
#include "proc.h"
#include "msr.h"
#include "kheap.h"

u8 ncpu;

Processor* GetCurrentProcessorByLapicid(){
	u64 rflags=SaveFlagsCli();
	u32 id=GetLapicId();
	Processor* cpu=NULL;
	for(int i=0;i<ncpu;i++){
		if(cpus[i].lapicid==id){
			cpu=cpus+i;
			break;
		}
	}
	LoadFlags(rflags);
	return cpu;
}
Processor* GetCurrentProcessor(){
	return cpus+GetCurrentProcess()->curcpu;
}

Bool rsdt64;
paddr_t rsdt_phy;
vaddr_t rsdt_lin;

Madt* GetMadt(BootArguments* bargs){
    set_mapping(ACPI_PAGE0, bargs->rsdp_phy, 0);
	Rsdptr* p = ACPI_PAGE0 + page4koff(bargs->rsdp_phy);
	if(p->ver == 0){
		rsdt_phy = p->rsdt_addr;
		rsdt64 = False;
		rsdt_lin = ACPI_PAGE0 + page4koff(p->rsdt_addr);
		set_mapping(ACPI_PAGE0, p->rsdt_addr, 0);
		Rsdt* rsdt = rsdt_lin;
		int cnt = (rsdt->head.len - sizeof(SdtHead)) / 4;
		for(int i = 0; i < cnt; i++){
			u64 phy = rsdt->ptrs[i];
			u64 lin = ACPI_PAGE1 + page4koff(phy);
			set_mapping(lin, phy, 0);
            SdtHead* head = lin;
            if(*(u32*)head->sign == *(u32*)"APIC"){
                return lin;
            }
		}
	}
	else{
		rsdt_phy = p->xsdt_addr;
		rsdt64 = True;
		rsdt_lin = ACPI_PAGE0 + page4koff(p->xsdt_addr);
		set_mapping(ACPI_PAGE0, p->xsdt_addr, 0);
		Xsdt* xsdt = rsdt_lin;
		int cnt = (xsdt->head.len - sizeof(SdtHead)) / 8;
		for(int i = 0; i < cnt; i++){
			u64 phy = xsdt->ptrs[i];
			u64 lin = ACPI_PAGE1 + page4koff(phy);
			set_mapping(lin, phy, 0);
            SdtHead* head = lin;
            if(*(u32*)head->sign == *(u32*)"APIC"){
                return lin;
            }
		}
	}
}
void delay(u32 n){
	while(n--){
		for(int i=0;i<0x1000;i++){
			vasm("nop");
		}
	}
}
void __startcpu(int dest){
	//bochsdbg();
	IpiCommand ipi = {
		.drvlev = True,
		.trigmod = 0,
		.delivmod = IpiInit
	};
	SendIpi(dest, ipi);
	delay(200);
	ipi.delivmod = IpiStartup;
	ipi.vector = 0x8000>>12;
	SendIpi(dest, ipi);
	delay(200);
	SendIpi(dest, ipi);
}
u32 bsplapicid;
Bool startcpu(Processor* dest){
    if(dest->lapicid == bsplapicid){
        dest->stat = CPU_READY;
        smp_init_self(dest);
        return True;
    }
    dest->stat = CPU_INIT;
    __startcpu(dest->lapicid);
    delay(1024);
    int i;
    for(i = 0; i < 4 && (dest->stat != CPU_READY); i++){
        __startcpu(dest->lapicid);
        delay(10240);
    }
    if(i == 4){
        dest->stat = CPU_GONE;
        return False;
    }
    return True;
}
void ParseMadt(Madt* madt){
	MadtEntry* e = madt->entries;
	u32 len = madt->head.len;
    bsplapicid = GetLapicId();
	while((u64)e-(u64)madt<len){
		if(e->type==0){
			struct _me_type0_lapic* me=e->data;
			Processor* p = cpus + ncpu;
            p->lapicid = me->lapicid;
			p->index = ncpu;
            if(startcpu(p)) ncpu++;
		}
		else if(e->type==9){
			struct _me_type9_x2apic* me=e->data;
			Processor* p = cpus + ncpu;
			p->lapicid = me -> x2apicid;
			p->index = ncpu;
            if(startcpu(p)) ncpu++;
		}
		e=(u64)e+e->len;
	}
}
void set_segmdesc(Segment* tab,int no,
	int base,int limit,
	Bool acs,Bool rw,Bool dir,Bool exec,int dpl
){
	Segment* d=tab+no;
	if((uint)limit>0xfffff){
		d->pagesized=True;
		limit>>=12;
	}
	d->limit=limit&0xffff;
	d->base=base&0xffff;
	d->base2=(base>>16)&0xff;
	d->gdt=True;
	d->present=True;
	d->access=acs;
	d->rw=rw;
	d->direction=dir;
	d->exec=exec;
	d->dpl=dpl;
	d->limit2=(limit>>16)&0xf;
	d->size32=True;
	d->base3=(base>>24)&0xff;
}
void prepare_tss(Processor* p){
	u32 dof = 10 + p->index * 2;
	u64 base = GDT + dof * sizeof(Segment);
	u64 addr = &p->tss;
	p->tss.ist1 = KERNEL_BASE + 0x44000 + p->index * 0x400;
	Segment *d = base;
	Gate* g = base;
	d->limit=sizeof(Tss64)-1;
	d->base=addr&0xffff;
	d->base2=(addr>>16)&0xff;
	g->type=TssGate;
	g->sys=False;g->dpl=False;g->present=True;
	d->limit2=0;
	d->avl=False;d->size32=False;d->size64=False;d->pagesized=False;
	d->base3=(addr>>24)&0xff;
	g->off3=addr>>32;
	g->pad=0;
	vasm("movl %0,%%eax;ltr %%ax;"::"g"(dof*8));
	p->tss.iobase=0;
}
void smp_init_self(Processor* self){
    prepare_tss(self);
	self->ready.list=kheap_alloc_zero(sizeof(Process*)*32);
    self->ready.cnt=0;
    self->ready.cur=0;
    init_spinlock(&self->ready.lock);
}
void IdleRoutine(){
    sti();
	while(1) hlt();
}
void apmain_asm();
void apmain(Processor* self){
    lapic_init();
    smp_init_self(self);
    // bochsdbg();
	char* name = "Idle0";
	name[4] = '0' + self->index;
	Process* idle = alloc_process(name);
	idle->vm = refer_vmspace(&kernmap);
	self->idle = idle;
	idle->curcpu = self->index;
	self->cur = idle;
	idle->stat = Running;
	WriteMSR(Fsbase, idle);
    self->stat = CPU_READY;
	IdleRoutine();
}
void smp_init(BootArguments* bargs){
    ncpu = 0;
    *(u64*)0x7e00 = apmain_asm;
    Madt* madt = GetMadt(bargs);
    ParseMadt(madt);
}