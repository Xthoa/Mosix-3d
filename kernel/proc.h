#pragma once

#include "types.h"
#include "vmem.h"
#include "vfs.h"
#include "pmem.h"
#include "except.h"
#include "spin.h"
#include "mutex.h"

typedef struct s_Bitmap{
	u32 max;
	u32 size;
	char data[];
} Bitmap;

#define MAXPROC 1984

typedef enum e_ProcStatus{
	Created=1,	// Just creating
	Running=2,	// Now on CPU
	Ready=3,	// Be ready to run
//	Blocked=4,	// Blocked
	Suspend=5,	// Suspended or sleeping
	Stopped=6,	// Wholy stopped
//	Confirmed=7	// Creator is aware of stop
} ProcStatus;

#define VM_READ 1	// readable
#define VM_WRITE 2	// writable
#define VM_RW 3
#define VM_EXECUTE 4	// executable
#define VM_RX 5
#define VM_RWX 7
#define VM_COW 8	// copy-on-write, not yet written - to be copied on write
#define VM_SHARE 0x10	// shared/shareable - paddr=>sharedptr
#define VM_SWAP 0x20	// swapped - to be swapped back on acs
#define VM_NOCOMMIT 0x40	// not commited(reserved) - to be allocated on acs

#define VM_IMAGE 1	// content of image
#define VM_LIBIMAGE 2	// content of library image
#define VM_STACK 4
#define VM_HEAP 8
#define VM_DATA 0x10	// (allocated space) private data

typedef struct s_SharedVmarea{
	paddr_t paddr;
	uint32_t ref;
} SharedVmarea;
typedef struct s_Vmarea{
	vaddr_t vaddr;
	union{
		paddr_t paddr;
		SharedVmarea* sharedptr;
	};
	uint32_t pages;
	uint16_t type;
	uint16_t flag;
} Vmarea;
typedef struct s_Vmspace{
	paddr_t cr3;
	pml4t_t pml4t;

	Vmarea* areas;
	uint32_t count;

	uint32_t ref;	// reference count
	Spinlock alock;
} Vmspace;	// work as mm_struct in linux

typedef struct s_Process{
	Vmspace* vm;

	u64 rsp;	// RSP in context switch
	vaddr_t rsb;	// stack position
	u16 sl;		// reserved stack (by pages)

	u8 curcpu;		// currently running on [USED BY ASSEMBLY, 0x1a]
	u8 lovedcpu;	// suggested processor
	u16 affinity;	// for every bit 1=allowance

	u16 pid;
	char* name;

	u64 fsbase;	// 0x28
	u64 gsbase;

	File** fdtable;

	jmp_buf* jbstack;
	u16 jbesp;
	
	volatile u16 stat;
	Waitlist waiter;
} Process;

extern Process** pidmap;

extern Vmspace kernmap;

Vmspace* alloc_vmspace();
void free_vmspace(Vmspace* map);
Vmspace* refer_vmspace(Vmspace* map);
void deref_vmspace(Vmspace* map);

void insert_vmarea(Vmspace* map, vaddr_t vaddr, paddr_t paddr, u32 pages, u16 type, u16 flag);
void delete_vmarea(Vmspace* map, vaddr_t vaddr);
Vmarea* find_vmarea(Vmspace* map, vaddr_t vaddr);

Process* alloc_process(char* name);
void free_process(Process* p);
Process* find_process(char* name);

void alloc_stack(Process* t, u16 rsv, u16 commit, vaddr_t top, vaddr_t entry);

Process* create_process(char* name, Vmspace* vm, u16 stkrsv, u16 stkcommit, vaddr_t stktop, vaddr_t entry);

int ready_process(Process* t);
void wait_process(Process* t);
void suspend_process();
void exit_process();

int fork_process();

Process* GetCurrentProcess();
u16 GetCurrentProcessID();
