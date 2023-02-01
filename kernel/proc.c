#include "proc.h"
#include "kheap.h"
#include "smp.h"
#include "msr.h"
#include "intr.h"
#include "asm.h"
#include "apic.h"
#include "macros.h"
#include "string.h"
#include "timer.h"
#include "bitmap.h"

PRIVATE Bitmap* procmap;
PUBLIC Vmspace kernmap;
PUBLIC Process** pidmap;

void proc_init(){
	procmap = KERNEL_BASE + 0x65e00;
	procmap->max = MAXPROC;
	procmap->size = 0;
	init_spinlock(&procmap->lock);
	pidmap = KERNEL_BASE + 0x62000;
	memset(pidmap, 0, 0x3e00);
	kernmap.cr3 = KERNEL_PML4_PHY;
	kernmap.pml4t = KERNEL_PML4;
	kernmap.ref = 0;
	kernmap.areas = NULL;
	kernmap.count = 0;
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

// process frame
Process* alloc_process(char* name){
	Process* p = kheap_alloc(sizeof(Process));
	p->fsbase = p;
	p->name = kheap_clonestr(name);
	p->pid = alloc_bit(procmap);
	pidmap[p->pid] = p;
	p->stat = Created;
	p->affinity = 0xffff;
	p->vm = NULL;
	p->jbesp = 0;
	p->lovedcpu=p->curcpu = 0;
	p->jbstack = kheap_alloc(sizeof(jmp_buf) * 32);
	init_dispatcher(&p->waiter, DISPATCH_PROCESS);
	p->waitcnt = 0;
	p->waitings = NULL;
	p->href = 1;
	init_spinlock(&p->rundown);
	return p;
}
void free_process(Process* p){
	kheap_free(p->jbstack);
	free_dispatcher(p);
	pidmap[p->pid] = NULL;
	free_bit(procmap, p->pid);
	kheap_freestr(p->name);
	kheap_free(p);
}
Process* find_process(char* name){
	for(int i=0;i<MAXPROC;i++){
		Process* p=pidmap[i];
		if(!p)continue;
		if(!strcmp(p->name,name))return p;
	}
	return NULL;
}

// vmspace: memory(map) resource
Vmspace* alloc_vmspace(){
	Vmspace* map = kheap_alloc(sizeof(Vmspace));
	map->pml4t = malloc_page4k_attr(1, PGATTR_NOEXEC);
	map->pml4t[511] = ((pml4t_t)KERNEL_PML4)[511];
	set_pml4e(map->pml4t + 510, kernel_v2p(map->pml4t), PGATTR_NOEXEC);
	memset(map->pml4t, 0, 510*8);
	map->cr3 = get_mapped_phy(map->pml4t);
	map->ref = 0;
	map->areas = malloc_page4k_attr(1, PGATTR_NOEXEC);	// max 85 records
	map->count = 0;
	init_spinlock(&map->alock);
	return map;
}
void free_vmspace(Vmspace* map){
	free_page4k(map->areas, 1);
	free_page4k(map->pml4t, 1);
	kheap_free(map);
}
Vmspace* refer_vmspace(Vmspace* map){
	lock_inc(map->ref);
	return map;
}
void deref_vmspace(Vmspace* map){
	lock_dec(map->ref);
	if(map->ref == 0)free_vmspace(map);
}
void insert_vmarea(Vmspace* map, vaddr_t vaddr, paddr_t paddr, u32 pages, u16 type, u16 flag){
	acquire_spin(&map->alock);
	Vmarea* vm = map->areas;
	for(int i=0; i<map->count; i++, vm++){
		if(vm->vaddr < vaddr) continue;
		if(vm->vaddr < vaddr+pages){
			release_spin(&map->alock);
			__throw(1);
		}
		push_back_array(map->areas, map->count, i, Vmarea);
		break;
	}
	map->count ++;
	vm->vaddr = vaddr;
	vm->paddr = paddr;
	vm->pages = pages;
	vm->type = type;
	vm->flag = flag;
	release_spin(&map->alock);
}
void delete_vmarea(Vmspace* map, vaddr_t vaddr){
	acquire_spin(&map->alock);
	for(int i=0; i<map->count; i++){
		Vmarea* vm = map->areas + i;
		if(vm->vaddr == vaddr){
			pull_back_array(map->areas, map->count, i, Vmarea);
			release_spin(&map->alock);
			return;
		}
	}
	release_spin(&map->alock);
	ASSERT_ARG(vaddr, "0x%q", vaddr);
	__throw(2);
}
Vmarea* find_vmarea(Vmspace* map, vaddr_t vaddr){
	acquire_spin(&map->alock);
	Vmarea* ret = NULL;
	for(int i=0; i<map->count; i++){
		Vmarea* vm = map->areas + i;
		if(vm->vaddr == vaddr){
			ret = vm;
			break;
		}
	}
	release_spin(&map->alock);
	return ret;
}

// memory(stack) resource
void map_initial_stack(Process* p, vaddr_t lin, paddr_t phy){
	pml4t_t pml4 = p->vm->pml4t;
	off_t pml4o = (lin>>39)&0x1ff;	// bit 39-47
	pdpt_t pdpt = malloc_page4k_attr(1, PGATTR_NOEXEC);
	memset(pdpt, 0, PAGE_SIZE);
	set_pml4e(pml4 + pml4o, kernel_v2p(pdpt), PGATTR_USER);
	off_t pdpto = (lin>>30)&0x1ff;	// bit 30-38
	pd_t pd = malloc_page4k_attr(1, PGATTR_NOEXEC);
	memset(pd, 0, PAGE_SIZE);
	set_pdpte(pdpt + pdpto, kernel_v2p(pd), PGATTR_USER);
	off_t pdo = (lin>>21)&0x1ff;	// bit 21-29
	pt_t pt = malloc_page4k_attr(1, PGATTR_NOEXEC);
	memset(pt, 0, PAGE_SIZE);
	set_pde(pd + pdo, kernel_v2p(pt), PGATTR_USER);
	off_t pto = (lin>>12)&0x1ff;	// bit 12-20
	set_pte(pt + pto, phy, PGATTR_NOEXEC);
}
void ProcessEntryStub();
void alloc_stack(Process* t, u16 rsv, u16 commit, vaddr_t top, vaddr_t entry){
	t->sl = rsv;
	t->rsb = top - rsv*PAGE_SIZE;
	t->rsp = top;
	paddr_t phy = alloc_phy(commit);	// paddr of commited stack
	paddr_t ptop = phy + commit*PAGE_SIZE;
	vaddr_t cbase = top - commit*PAGE_SIZE;	// base vaddr of commited stack
	vaddr_t mbase = top - PAGE_SIZE;	// base vaddr of stack page0
	if(rsv > commit){
		insert_vmarea(t->vm, t->rsb, 0, rsv - commit, VM_STACK, VM_RW|VM_NOCOMMIT);
	}
	insert_vmarea(t->vm, cbase, phy, commit, VM_STACK, VM_RW);
	u64* rsp;
	if(t->vm->ref == 1){
		map_initial_stack(t, mbase, ptop - PAGE_SIZE);
		set_mapping(kernel_p2v(ptop - PAGE_SIZE), ptop - PAGE_SIZE, 0);
		rsp = kernel_p2v(ptop);
	}
	else{
		set_mappings(cbase, phy, commit, PGATTR_NOEXEC);
		rsp = top;
	}
	t->rsp -= 0x38;
	rsp[-1] = ProcessEntryStub;
	rsp[-2] = entry;
	rsp[-3] = t;
}

Bool scan_del_pgtab(vaddr_t base, vaddr_t stack){
	vaddr_t top = base + (base == SELF_REF4_ADDR ? PAGE_SIZE/2 : PAGE_SIZE);
	Bool res = False, deep = ((base >> 21ull) & 0x7ffffff) == 0x7fbfdfe;
	for(vaddr_t scan = base; scan < top; scan += 8){
		Bool ms = False;
		u64 pte = *(u64*)scan;
		if(pte & 1){
			if(deep){
				vaddr_t next = (scan << 9) | CANONICAL_SIGN;
				ms = scan_del_pgtab(next, stack);
			}
			else ms = (pte == stack);
			u64 phy = get_page_phy(*(pte_t*)&pte);
			if(!ms) free_phy(phy, 1);
			res = (res || ms);
		}
	}
	return res;
}
// entry stub: memory(code) resource
void ProcessEntrySafe(u64 routine, Process* self){
	if(self->vm->ref == 1){
		Vmspace* vm = self->vm;
		for(int i = 0; i < vm->count; i++){
			Vmarea* a = vm->areas + i;
			if(a->flag & VM_NOCOMMIT) continue;
			uint32_t pgattr = 0;
			if((a->flag & VM_WRITE) == 0) pgattr |= PGATTR_READONLY;
			if((a->flag & VM_EXECUTE) == 0) pgattr |= PGATTR_NOEXEC;
			if(a->flag & VM_SHARE){
				SharedVmarea* sa = a->sharedptr;
				set_mappings(a->vaddr, sa->paddr, a->pages, pgattr);
			}
			else set_mappings(a->vaddr, a->paddr, a->pages, pgattr);
		}
	}
	int code;
	__try{
		int (*func)()=routine;
		func();
	}
	__catch(code){
		// printk("Caught exception %d\n",code);
		// dump_context();
	}
	if(self->vm->ref == 1){
		Vmspace* vm = self->vm;
		vaddr_t stack;
		for(int i = 0; i < vm->count; i++){
			Vmarea* a = vm->areas + i;
			if(a->type == VM_STACK){
				if((a->flag & VM_NOCOMMIT) == 0){	// commited stack size
					self->rsp = a->paddr;
					self->sl = a->pages;
				}
				continue;
			}
			if(a->flag & VM_SHARE){
				SharedVmarea* sa = a->sharedptr;
				lock_dec(sa->ref);	// TODO
			}
			free_phy(a->paddr, a->pages);
		}
		u64 rsp = self->rsb + self->sl * PAGE_SIZE - 1;
		scan_del_pgtab(SELF_REF4_ADDR, *(u64*)get_mapping_pde(rsp));
	}
	free_htab(self);
	exit_process();
}

// initialized process creator
Process* create_process(char* name, Vmspace* vm, 
		u16 stkrsv, u16 stkcommit, 
		vaddr_t stktop, vaddr_t entry){
	Process* p = alloc_process(name);
	if(vm == NULL) vm = alloc_vmspace();
	p->vm = refer_vmspace(vm);
	alloc_htab(p);
	alloc_stack(p, stkrsv, stkcommit, stktop, entry);
	return p;
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
			pull_back_array(rl->list, rl->cnt, i, Process*);
			rl->cnt--;
			break;
		}
	}
	release_spin(&rl->lock);
}
Processor* choose_cpu_for(Process* t){
	Processor* min=cpus;	// cpu0 by default
	for(int i=1;i<ncpu;i++){
		Processor* p=cpus+i;
		if(p->stat!=2)continue;
		if(!((t->affinity>>i)&1))continue;
		if(p->ready.cnt<min->ready.cnt)min=p;
	}
	return min;
}

// Scheduler
void sched(){
	Processor* p = GetCurrentProcessorByLapicid();
	if(p->tickleft){
		p->tickleft --;
		return;
	}
	Process* now = p->cur;
	ReadyList* r = &p->ready;
	acquire_spin(&r->lock);
	if(r->cnt > 1 || now == p->idle && r->cnt > 0){
		r->cur++;
		if(r->cur==r->cnt)r->cur=0;
		Process* next=r->list[r->cur];
		release_spin(&r->lock);
		if(!next){
			// puts("Fatal Panic: next process is NULL\n");
			// dump_context();
			hlt();
		}
		p->cur = next;
		p->tickleft = 1;
		now->stat = Ready;
		next->stat = Running;
		WriteMSR(Fsbase, next->fsbase);
		switch_context(now, next);
	}
	else release_spin(&r->lock);
}
void sched_suspend(){
	Processor* p = GetCurrentProcessorByLapicid();
	Process* now = p->cur;
	ReadyList* r = &p->ready;
	Process* next = p->idle;
	acquire_spin(&r->lock);
	if(r->cur == r->cnt) r->cur=0;
	if(r->cnt > 1) next = r->list[r->cur];
	release_spin(&r->lock);
	p->cur = next;
	p->tickleft = 1;
	now->stat = Suspend;
	next->stat = Running;
	WriteMSR(Fsbase, next->fsbase);
	switch_context(now, next);
}
void sched_exit(){
	Processor* p = GetCurrentProcessorByLapicid();
	Process* now = p->cur;
	ReadyList* r = &p->ready;
	Process* next = p->idle;
	acquire_spin(&r->lock);
	if(r->cur == r->cnt) r->cur=0;
	if(r->cnt > 1) next = r->list[r->cur];
	release_spin(&r->lock);
	p->cur = next;
	p->tickleft = 1;
	now->stat = Stopped;
	next->stat = Running;
	WriteMSR(Fsbase, next->fsbase);
	switch_context_exit(now, next, &now->stat);
}

int ready_process(Process* t){
	Processor* p=cpus;
	p = choose_cpu_for(t);
	ReadyList* r = &p->ready;
	if(r->cnt >= 32)return ErrFull;
	u64 rfl = SaveFlagsCli();	// raise irql to dispatch-level
	add_ready(t, r);
	LoadFlags(rfl);
	t->curcpu = p->index;
	t->stat = Ready;
	return Success;
}
void wait_process(Process* t){
	if(t->stat == Stopped) return;
	acquire_spin(&t->waiter.lock);
	u32 tmp = t->waiter.count++;
	t->waiter.list[tmp] = GetCurrentProcess();
	release_spin(&t->waiter.lock);
	while(t->stat != Stopped){
		suspend_process();
	}
}
void suspend_process(){
	u64 rfl = SaveFlagsCli();	// raise irql to dispatch-level
	Process* now = GetCurrentProcess();
	del_ready(&cpus[now->curcpu].ready, now);
	sched_suspend();
	LoadFlags(rfl);
}
void exit_process(){
	cli();	// raise irql to dispatch-level
	Process* now = GetCurrentProcess();
	del_ready(&cpus[now->curcpu].ready, now);
	acquire_spin(&now->waiter.lock);
	for(int i = 0; i < now->waiter.count; i++){
		ready_process(now->waiter.list[i]);
	}
	release_spin(&now->waiter.lock);
    lock_dec(now->href);
    if(now->href == 0) reap_process(now);
	sched_exit();
}
void reap_process(Process* p){
	Message msg;
	msg.type = SPMSG_REAP;
	msg.arg64 = p;
	send_message(sysprocml, &msg);
}
void do_reap_process(Process* p){
	vaddr_t slin = p->rsb;
	pml4t_t pml4t = p->vm->pml4t;
	off_t pml4o = extract_pml4o_macro(slin);
	paddr_t pdptp = get_pdpt_phy(pml4t[pml4o]);
	pdpt_t pdpt = kernel_p2v(pdptp);
	set_mapping(pdpt, pdptp, 0);
	off_t pdpto = extract_pdpto_macro(slin);
	paddr_t pdp = get_pd_phy(pdpt[pdpto]);
	pd_t pd = kernel_p2v(pdp);
	set_mapping(pd, pdp, 0);
	off_t pdo = extract_pdo_macro(slin);
	paddr_t ptp = get_pt_phy(pd[pdo]);
	free_phy(pdptp, 1);
	free_phy(pdp, 1);
	free_phy(ptp, 1);
	deref_vmspace(p->vm);
	paddr_t sphy = p->rsp;
	u32 sl = p->sl;
	free_phy(sphy, sl);
	free_process(p);
}

IntHandler void sched_tick_ipistub(IntFrame* f){
	WriteEoi();
	sched();
}
/*IntHandler void sched_stop_ipistub(IntFrame* f){
	WriteEoi();
	exit_process();
}*/
void sched_init(){
	set_gatedesc(0x38,sched_tick_ipistub,8,0,3,Interrupt);
	//set_gatedesc(0x3a,sched_stop_ipistub,8,0,3,Interrupt);
}
