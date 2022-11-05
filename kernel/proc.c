#include "proc.h"
#include "kheap.h"
#include "smp.h"
#include "msr.h"
#include "intr.h"
#include "vfs.h"
#include "asm.h"
// #include "exec.h"
#include "apic.h"
#include "vmem.h"
#include "spin.h"

// Bitmap
off_t alloc_bit(Bitmap* map){
	u32 bytes=(map->max+7)>>3;
	for(int i=0;i<bytes;i++){
		u8 c = map->data[i];
		if(c == 0xff)continue;
		for(int j=0;j<8;j++){
			Bool bit=(c>>j)&1;
			if(bit==False){
				map->data[i]|=(1u<<j);
				return i*8+j;
			}
		}
	}
	map->size++;
	return map->max;
}
void free_bit(Bitmap* map,off_t bit){
	map->data[bit>>3]&=~(1u<<(bit&3));
	map->size--;
}

PRIVATE Bitmap* procmap;
PUBLIC Pagemap kernmap;
PUBLIC Process** pidmap;
// PRIVATE Node* procor;

void proc_init(){
	procmap=KERNEL_BASE+0x65e00;
	procmap->max=1984;
	procmap->size=0;
	pidmap=KERNEL_BASE+0x62000;
	memset(pidmap, 0, 0x3e00);
	kernmap.cr3 = KERNEL_PML4_PHY;
	kernmap.pml4t = KERNEL_PML4;
	kernmap.ref = 0;
	// procor = create_subdir(find_node("/run"), "proc", 0);
}

Process* GetCurrentProcess(){
	u64 val;
	val=ReadMSR(Fsbase);
	return val;
}
u16 GetCurrentProcessID(){
	Process* t=NULL;
	u16 val;
	asm("movw %%fs:%1,%0":"=g"(val):"g"(t->pid));
	return val;
}

Pagemap* alloc_pagemap(){
	Pagemap* map = kheap_alloc(sizeof(Pagemap));
	map->pml4t = malloc_page4k(1);
	map->cr3 = get_mapped_phy(map->pml4t);
	map->ref = 0;
	return map;
}
void free_pagemap(Pagemap* map){
	free_page4k(map->pml4t, 1);
	kheap_free(map);
}
Pagemap* refer_pagemap(Pagemap* map){
	asm("lock; incl %0":"+m"(map->ref));
	return map;
}
void deref_pagemap(Pagemap* map){
	asm("lock; decl %0":"+m"(map->ref));
	if(map->ref == 0)free_pagemap(map);
}

Process* alloc_process(char* name){
	Process* p = kheap_alloc(sizeof(Process));
	p->fsbase = p;
	p->name = kheap_clonestr(name);
	/*Object* o=create_object(p, sizeof(Process), proctp);
	Node* n=create_node(procor, p->name, o);
	p->node=n;*/
	p->pid = alloc_bit(procmap);
	pidmap[p->pid] = p;
	p->stat = Created;
	p->affinity = 0xffff;
	p->pagemap = NULL;
	p->father=p->prev=p->next=p->child = NULL;
	p->jbesp = 0;
	p->lovedcpu=p->curcpu = 0;
	p->jbstack = kheap_alloc(sizeof(jmp_buf) * 32);
	init_spinlock(&p->treelock);
	return p;
}
void free_process(Process* p){
	/*destroy_object(p->node->obj);
	destroy_node(p->node);*/
	kheap_free(p->jbstack);
	pidmap[p->pid] = NULL;
	free_bit(procmap, p->pid);
	kheap_freestr(p->name);
	kheap_free(p);
}
void set_process_father(Process* p,Process* f){
	p->prev=NULL;
	p->father=f;
	acquire_spin(&f->treelock);
	p->next=f->child;
	if(f->child)f->child->prev=p;
	f->child=p;
	release_spin(&f->treelock);
}
void del_process_father(Process* p){
	acquire_spin(&p->treelock);
	Process* f=p->father;
	if(!f)return;
	acquire_spin(&f->treelock);
	if(f->child==p)f->child=p->next;
	else p->prev->next=p->next;
	release_spin(&f->treelock);
	if(p->next)p->next->prev=p->prev;
	release_spin(&p->treelock);
}
void be_process_father(Process* p){
	set_process_father(p,GetCurrentProcess());
}
Process* create_process(char* name){
	Process* p=alloc_process(name);
	be_process_father(p);
	return p;
}
void destroy_process(Process* p){
	del_process_father(p);
	free_process(p);
}
Process* find_process(char* name){
	for(int i=0;i<MAXPROC;i++){
		Process* p=pidmap[i];
		if(!p)continue;
		if(!strcmp(p->name,name))return p;
	}
	return NULL;
}

Bool alloc_stack(Process* t,u16 sl){
	if(!t)return ErrNull;
	if(!sl)return ErrNull;
	t->sl = sl;
	t->rsb = malloc_page4k_attr(sl, PGATTR_NOEXEC);
	// Stack execution protection
	t->rsp = t->rsb+sl*PAGE_SIZE;
	return Success;
}
Bool free_stack(Process* t){
	if(!t)return ErrNull;
	if(!t->sl)return ErrNull;
	free_page4k(t->rsb,t->sl);
	t->rsb=t->sl=0;
	return Success;
}
void ProcessEntryStub();
void ProcessEntrySafe(u64 routine,Process* self){
	int code;
	__try{
		int (*func)()=routine;
		func();
	}
	__catch(code){
		// printk("Caught exception %d\n",code);
		// dump_context();
	}
	exit_process();
}
Bool set_process_entry(Process* t,u64 routine){
	if(!t)return ErrNull;
	u64 *rsp=t->rsp;
	t->rsp-=0x38;
	rsp[-1]=ProcessEntryStub;
	rsp[-2]=routine;
	rsp[-3]=t;
	return Success;
}

void alloc_fdtable(Process* p){
	p->fdtable = kheap_alloc_zero(sizeof(File*) * 16);
}
void free_fdtable(Process* p){
	kheap_free(p->fdtable);
}

// Ready list stuff
void add_ready(Process* t, ReadyList* rl){
	acquire_spin(&rl->lock);
	rl->list[rl->cnt++]=t;
	release_spin(&rl->lock);
}
void del_ready(ReadyList* rl, Process* t){
	acquire_spin(&rl->lock);
	for(int i=0; i<rl->cnt; i++){
		if(rl->list[i] == t){
			memmove(rl->list+i, rl->list+i+1, (rl->cnt-i-1)*sizeof(Process*));
			rl->cnt--;
			break;
		}
	}
	release_spin(&rl->lock);
}
Processor* choose_cpu_for(Process* t){
	Processor* min=cpus;	// cpu0 by default
	for(int i=0;i<ncpu;i++){
		Processor* p=cpus+i;
		if(p->stat!=2)continue;
		if(!((t->affinity>>i)&1))continue;
		if(p->ready.cnt<min->ready.cnt)min=p;
	}
	return min;
}

// Scheduler
void switch_to(Processor* p,Process* now,Process* next){
	p->cur=next;
	p->tickleft=1;
	next->stat=Running;
	/*
	__builtin_ia32_wrfsbase64(next->fsbase)
	//__builtin_ia32_wrgsbase64(next->gsbase)
	*/	// CPUID[0X7].EBX[0], CR4[16]
	WriteMSR(Fsbase,next->fsbase);
	//WriteMSR(Gsbase,next->gsbase);
	// my lenovo ideapad doesnt support gsbase
	switch_context(now,next);
}
int sched(){
	Processor* p=GetCurrentProcessorByLapicid();
	if(p->tickleft){
		p->tickleft--;
		return ErrReject;
	}
	Process* now=p->cur;
	ReadyList* r=&p->ready;
	if(r->cnt>1 || now==p->idle && r->cnt>0){
		acquire_spin(&r->lock);
		r->cur++;
		if(r->cur==r->cnt)r->cur=0;
		Process* next=r->list[r->cur];
		release_spin(&r->lock);
		if(!next){
			// puts("Fatal Panic: next thread is NULL\n");
			// dump_context();
			hlt();
		}
		now->stat=Ready;
		switch_to(p,now,next);
		return Success;
	}
	else return ErrNotFound;
}
int sched_away(){
	Processor* p=GetCurrentProcessorByLapicid();
	Process* now=p->cur;
	ReadyList* r=&p->ready;
	if(r->cnt>1){	// now must != idle
		acquire_spin(&r->lock);
		r->cur++;
		if(r->cur==r->cnt)r->cur=0;
		Process* next=r->list[r->cur];
		release_spin(&r->lock);
		switch_to(p,now,next);
		return Success;
	}
	else switch_to(p,now,p->idle);
}

int ready_process(Process* t){
	if(!t)return ErrNull;
	Processor* p=cpus;
	p = choose_cpu_for(t);
	ReadyList* r = &p->ready;
	if(r->cnt >= 32)return ErrFull;
	add_ready(t, r);
	t->curcpu = p->index;
	t->stat = Ready;
	return Success;
}
void wait_process(Process* t){
	while(t->stat != Stopped){
		suspend_process();
	}
}
void suspend_process(){
	Process* t = GetCurrentProcess();
	del_ready(&cpus[t->curcpu].ready, t);
	t->stat = Suspend;
	u64 rfl = SaveFlagsCli();
	sched_away();
	LoadFlags(rfl);
}
void exit_process(){
	cli();
	Process* now = GetCurrentProcess();
	now->stat = Stopped;
	return sched_away();
}

IntHandler void sched_tick_ipistub(IntFrame* f){
	WriteEoi();
	sched();
}
IntHandler void sched_stop_ipistub(IntFrame* f){
	WriteEoi();
	exit_process();
}
void sched_init(){
	set_gatedesc(0x38,sched_tick_ipistub,16,0,3,Interrupt);
	set_gatedesc(0x3a,sched_stop_ipistub,16,0,3,Interrupt);
}