#include "types.h"
#include "asm.h"
#include "intr.h"
#include "smp.h"
#include "proc.h"

// set a gate in the specified base as a gate table
void set_gatedesc_base(Gate* base,u8 no,
	vaddr_t off,u16 sel,u8 ist,
	u8 dpl,enum e_GateType type
){
	Gate* gt=base+no;
	gt->off=off&0xffff;
	gt->off2=off>>16;
	gt->off3=off>>32;
	gt->sel=sel;
	gt->ist=ist;
	gt->present=True;
	gt->dpl=dpl;
	gt->type=type;
	gt->sys=False;	//false: is sys desc
}
void set_gatedesc(u32 no,vaddr_t off,u16 sel,u8 ist,u8 dpl,enum e_GateType type){
	set_gatedesc_base(IDT,no,off,sel,ist,dpl,type);
}

// info dumping functions

void dump_gate(int n){
	Gate* g=IDT+n;
	u32* ug=g;
	printk("IntGate dump: index %b\n",n);
	printk("  gate %d_%d_%d_%d\n",g[0],g[1],g[2],g[3]);
	printk("    %c %c type=%B dpl=%B ist=%B\n",
		g->present?'P':'p',g->sys?'s':'S',g->type,g->dpl,g->ist);
	u64 off=g->off3;
	off=(off<<16)+g->off2;
	off=(off<<16)+g->off;
	printk("  target %w:%q\n",g->sel,off);
}
void dump_segment(int sel){
	printk("Segment dump: selector %w\n",sel);
	Segment* g=GDT+(sel>>3);
	u32* ug=g;
	printk("  descriptor %d_%d\n",ug[0],ug[1]);
	printk("    %c %c %c %c %c %B dpl=%b\n",
		g->present?'P':'p',g->avl?'s':'S',
		g->rw?'R':'r',g->exec?'X':'x',g->pagesized?'P':'B',
		g->size64?64:(g->size32?32:16),g->dpl);
	u32 base=g->base3;
	base=(base<<8)+g->base2;
	base=(base<<16)+g->base;
	u32 limit=g->limit2;
	limit=(limit<<16)+g->limit;
	if(g->pagesized)limit<<=12;
	printk("  base %d limit %d\n",base,limit);
}
/*
void dump_mem(u64 lin,u16 size){
	printk("Memory dump: %w bytes from %q\n",size,lin);
	u8 lc=size/16;
	u16 off=0;
	u8* ptr=lin;
	while(lc--){
		printk("  +0x%w:",off);
		for(int i=0;i<16;i++){
			printk(" %b",ptr[off+i]);
		}
		putc('\n');
		off+=16;
	}
	if(size-off){
		printk("  +0x%w:",off);
		for(int i=0;i<size-off;i++){
			printk(" %b",ptr[off+i]);
		}
	}
	putc('\n');
}*/
void dump_context(){
	puts("Context:\n  Processor ");
	Processor* p=GetCurrentProcessorByLapicid();
	Process* t=p->cur;
	printk("%B\n  Process '%s' id=%w at %q\n", p->index, t->name, t->pid, t);
}

IntHandler void interr0d(IntFrame* f,u64 code){
	bochsdbg();
	//global_color=WHITE;
	puts("\nSystem error: #GP General Protection\n");
	printk("  at %w:%q\n  rsp %p\n",f->cs,f->rip,f->rsp);
	if(code==0x113)puts("  Cause: Gate.sys = False\n");
	elif(code){
		puts("  Segmentation Fault\n");
		dump_segment(code);
	}
	dump_context();
	//if(dbg_regs)dump_regs(f->rsp);
	bochsdbg();
	//wait_reset();
	hlt();
}

//Bool tracing=False;
IntHandler void interr0e(IntFrame* f,u64 code){
	bochsdbg();
	//if(tracing)goto np;
	u64 cr2=getcr2();
	//global_color=WHITE;
	puts("\nSystem error: #PF Page Fault\n");
	printk("  at %w:%q\n  to %p\n  rsp %p\n",f->cs,f->rip,cr2,f->rsp);
	printk("%s of %s page in %s mode\n",
		code&2?"Write":"Read",code&1?"protected":"non-present",code&4?"user":"kernel");
	dump_context();
	/*puts("Page index route:\n");
	u64 cr3=getcr3();
	tracing=True;
	pml4_t pml4=core_phy2lin(cr3);
	off_t pml4o=extract_pml4o(cr2);
	pml4e_t pml4e=get_pml4_entry(pml4, pml4o);
	printk("  PML4 %q [%b] %q\n",pml4,pml4o,*(u64*)&pml4e);
	if(!pml4e.present)goto np;
	pdpt_t pdpt=core_phy2lin(get_pdpt_phy(pml4e));
	off_t pdpto=extract_pdpto(cr2);
	pdpte_t pdpte=get_pdpt_entry(pdpt, pdpto);
	printk("  PDPT %q [%b] %q\n",pdpt,pdpto,*(u64*)&pdpte);
	if(!pdpte.present)goto np;
	pd_t pd=core_phy2lin(get_pd_phy(pdpte));
	off_t pdo=extract_pdo(cr2);
	pde_t pde=get_pd_entry(pd, pdo);
	printk("  PD  %q [%b] %q\n",pd,pdo,*(u64*)&pde);
	if(!pde.present)goto np;
	pt_t pt=core_phy2lin(get_pt_phy(pde));
	off_t pto=extract_pto(cr2);
	pte_t pte=get_pt_entry(pt, pto);
	printk("  PT %q [%b] %q\n",pt,pto,*(u64*)&pte);
	if(!pte.present)goto np;
	u64 phy=get_page_phy(pte);
	printk("  Page %q : %q\n",cr2,pte);
	puts("End: access check\n");
	goto end;
	np:
	puts("End: pt not present\n");
	end:*/
	//if(dbg_regs)dump_regs(rsp);
	bochsdbg();
	//wait_reset();
	hlt();
}

IntHandler void interr08(IntFrame* f,u64 code){
	//global_color=RED;
	puts("FATAL");
	bochsdbg();
	hlt();
}

IntHandler void intall(IntFrame* f){
	//global_color=WHITE;
	puts("\nSystem error: Interrupt\n");
	printk("  at %w:%q\n",f->cs,f->rip);
	dump_context();
	//if(dbg_regs)dump_regs(rsp);
	bochsdbg();
	//wait_reset();
	hlt();
}
IntHandler void interr0a(IntFrame* f,u64 code){
	//global_color=WHITE;
	puts("\nSystem error: #TS Faulty TSS\n");
	printk("  at %w:%q\n  rsp %p\n  code %q\n",f->cs,f->rip,f->rsp);
	dump_context();
	bochsdbg();
	hlt();
}
IntHandler void interr0b(IntFrame* f,u64 code){
	//global_color=WHITE;
	puts("\nSystem error: #NP Segment not present\n");
	printk("  at %w:%q\n  rsp %p\n  code %q\n",f->cs,f->rip,f->rsp);
	dump_context();
	bochsdbg();
	hlt();
}
IntHandler void interr0c(IntFrame* f,u64 code){
	//global_color=WHITE;
	puts("\nSystem error: #SS Stack fault\n");
	printk("  at %w:%q\n  rsp %p\n  code %q\n",f->cs,f->rip,f->rsp);
	dump_context();
	bochsdbg();
	hlt();
}
void fault_intr_init(){
	//set_gatedesc(0x00,(u64)interr00,16,0,3,Interrupt);
	//set_gatedesc(0x03,int03,16,0,3,TrapGate);
	set_gatedesc(0x08,interr08,16,0,3,Interrupt);
	set_gatedesc(0x0a, interr0a, 16, 0, 3, Interrupt);
	set_gatedesc(0x0b, interr0b, 16, 0, 3, Interrupt);
	set_gatedesc(0x0c, interr0c, 16, 0, 3, Interrupt);
	set_gatedesc(0x0d,(u64)interr0d,16,0,3,Interrupt);
	set_gatedesc(0x0e,(u64)interr0e,16,0,3,Interrupt);
}
