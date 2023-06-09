#pragma once

#include "types.h"
#include "vmem.h"
#include "vfs.h"
#include "pmem.h"
#include "except.h"
#include "spin.h"
#include "mutex.h"
#include "handle.h"
#include "msglist.h"
#include "exec.h"

#define MAXPROC 1984

typedef enum e_ProcStatus{
	Created=1,	// Just creating
	Running=2,	// Now on CPU
	Ready=3,	// Be ready to run
//	Blocked=4,	// Blocked
	Suspend=5,	// Suspended or sleeping
	Stopped=6,	// exited
	Dead=7		// got down of cpu
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
#define VM_MAPPED_PADDR 0x80	// check paddr in page table

#define VM_IMAGE 1	// content of image
#define VM_LIBIMAGE 2	// content of library image
#define VM_STACK 4
#define VM_HEAP 8
#define VM_DATA 0x10	// (allocated space) private data

#define SPMSG_REAP 1

#define PENV_NATIVE 0
#define PENV_PE64 1
#define PENV_ELF64 2
#define PENV_DRIVE_PE64 3

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

typedef struct s_ActivePedllList ActivePedllList;
typedef struct s_Process{
	Vmspace* vm;	// [ASM 0x00]
	u64 rsp;	// RSP in context switch [ASM 0x08]
	vaddr_t rsb;	// stack position 0x10
	u16 sl;		// reserved stack (by pages) 0x18

	u8 curcpu;		// currently running on [ASM 0x1a]
	u8 lovedcpu;	// suggested processor [ASM 0x1b]
	u16 affinity;	// for every bit 1=allowance 0x1c

	u16 pid;	// 0x1e
	char* name;	// 0x20
	u64 fsbase;	// 0x28
	vaddr_t kstack;	// [ASM 0x30]
	u64 gsbase;

	Signal* deathsig;
	HandleTable htab;
	u16 jbesp;
	jmp_buf* jbstack;
	PeInfo peinfo;
	Path cwd;
	Freelist* heap;
	char* argv;

	volatile u16 stat;
	u16 href;	// handle reference count
	u16 errno;
	u8 env;
	Spinlock rundown;
} Process;

extern Process** pidmap;
extern Vmspace kernmap;
extern MessageList* sysprocml;

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
void reap_process(Process* p);
void do_reap_process(Process* p);

Process* GetCurrentProcess();
u16 GetCurrentProcessID();
