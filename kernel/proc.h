#pragma once

#include "types.h"
#include "vmem.h"
#include "vfs.h"
#include "pmem.h"
#include "except.h"
#include "spin.h"

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

struct s_Instance;
typedef struct s_Pagemap{
	paddr_t cr3;
	pml4t_t pml4t;
	uint32_t ref;	// reference count
} Pagemap;
typedef struct s_Process{
	Pagemap* pagemap;

	u64 rsp;
	u64 rsb;
	u16 sl;		// by pages

	u8 curcpu;		// currently running on [USED BY ASSEMBLY, 0x1a]
	u8 lovedcpu;	// suggested processor
	u16 affinity;	// for every bit 1=allowance

	u16 pid;
	char* name;

	u64 fsbase;	// 0x28
	u64 gsbase;

	struct s_Process* father;
	struct s_Process* prev;
	struct s_Process* next;
	struct s_Process* child;

	jmp_buf* jbstack;
	u16 jbesp;
	
	volatile u16 stat;
	Spinlock treelock;
} Process;

extern Process** pidmap;

extern Pagemap kernmap;

Pagemap* alloc_pagemap();
void free_pagemap(Pagemap* map);
Pagemap* refer_pagemap(Pagemap* map);
void deref_pagemap(Pagemap* map);

Process* alloc_process(char* name);
void free_process(Process* p);
Process* create_process(char* name);
void destroy_process(Process* p);
Process* find_process(char* name);

Bool alloc_stack(Process* t,u16 sl);
Bool set_process_entry(Process* t,u64 routine);

int ready_process(Process* t);
void wait_process(Process* t);
void suspend_process();
void exit_process();

int fork_process();

Process* GetCurrentProcess();
u16 GetCurrentProcessID();
struct s_Instance* GetCurrentInstance();
