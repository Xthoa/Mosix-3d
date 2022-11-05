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
	Processor* p = GetCurrentProcessorByLapicid();
	if(p == NULL){
		puts("Context not initialized (CPU==null)\n");
		return;
	}
	puts("Context:\n  Processor ");
	Process* t = p->cur;
	printk("%B\n  Process '%s' id=%w at %q\n", p->index, t->name, t->pid, t);
}

IntHandler void interr0d(IntFrame* f,u64 code){
	bochsdbg();
	puts("\nSystem error: #GP General Protection\n");
	printk("  at %w:%q\n  rsp %p\n",f->cs,f->rip,f->rsp);
	if(code==0x113)puts("  Cause: Gate.sys = False\n");
	elif(code){
		puts("  Segmentation Fault\n");
		dump_segment(code);
	}
	dump_context();
	bochsdbg();
	//wait_reset();
	hlt();
}

IntHandler void interr0e(IntFrame* f,u64 code){
	bochsdbg();
	u64 cr2=getcr2();
	puts("\nSystem error: #PF Page Fault\n");
	printk("  at %w:%q\n  to %p\n  rsp %p\n",f->cs,f->rip,cr2,f->rsp);
	printk("%s of %s page in %s mode\n",
		code&2?"Write":"Read",code&1?"protected":"non-present",code&4?"user":"kernel");
	dump_context();
	printk("PDPTE: %q\n",*(u64*)get_mapping_pdpte(cr2));
	printk("PDE:   %q\n",*(u64*)get_mapping_pde(cr2));
	printk("PTE:   %q\n",*(u64*)get_mapping_pte(cr2));
	bochsdbg();
	//wait_reset();
	hlt();
}

IntHandler void interr08(IntFrame* f,u64 code){
	puts("\nFatal error: #DF Double Fault\n");
	bochsdbg();
	hlt();
}

IntHandler void intall(IntFrame* f){
	puts("\nSystem error: Interrupt\n");
	printk("  at %w:%q\n",f->cs,f->rip);
	dump_context();
	bochsdbg();
	//wait_reset();
	hlt();
}
IntHandler void interr0a(IntFrame* f,u64 code){
	puts("\nSystem error: #TS Faulty TSS\n");
	printk("  at %w:%q\n  rsp %p\n  code %q\n",f->cs,f->rip,f->rsp);
	dump_context();
	bochsdbg();
	hlt();
}
IntHandler void interr0b(IntFrame* f,u64 code){
	puts("\nSystem error: #NP Segment not present\n");
	printk("  at %w:%q\n  rsp %p\n  code %q\n",f->cs,f->rip,f->rsp);
	dump_context();
	bochsdbg();
	hlt();
}
IntHandler void interr0c(IntFrame* f,u64 code){
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
