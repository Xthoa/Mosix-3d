#include "types.h"
#include "asm.h"
#include "intr.h"

void fault_intr_init(){
    // TODO: set gate descriptors for the x86 exceptions
}

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
/*
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
	Descriptor* g=GDT+(sel>>3);
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
}void dump_mem(u64 lin,u16 size){
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
}
void dump_context(){
	puts("Context:\n  Processor ");
	Processor* p=GetCurrentProcessorByLapicid();
	Thread* t=p->cur;
	printk("%B\n  Thread id=%w at %q\n",p->index,t->tid,t);
	printk("  Process '%s' id=%b at %q\n",t->proc->name,t->proc->pid,t->proc);
}
*/