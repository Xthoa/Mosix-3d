#include "types.h"
#include "asm.h"
#include "boot.h"
#include "proc.h"
#include "smp.h"
#include "msr.h"
#include "vfs.h"
#include "kheap.h"
#include "exec.h"

// Kernel C code starts from here.
// This is the entry point in kernel.elf
// which is loaded at (paddr_t)0x11000.
void test_proc();
void KernelBootEntry(BootArguments* bargs){
	/*
	1. init memory & kheap
	2. init interrupts & exceptions
	3. init vfs
	4. init timer
	5. init smp & proc
	6. start sched
	*/
	cga_init();
	puts("Mosix 3d booting...\n");
	kheap_init();
	pmem_init(bargs);
	fault_intr_init();
	ioapic_init();
	lapic_init();
	vfs_init();
	timer_init();
	proc_init();
	sched_init();
	smp_init(bargs);
	Processor* c = GetCurrentProcessorByLapicid();
	Process* idle = alloc_process("Idle");
	idle->vm = refer_vmspace(&kernmap);
	c->idle = idle;
	idle->curcpu = c->index;
	c->cur = idle;
	idle->stat = Running;
	WriteMSR(Fsbase, idle);

	mount_initfs();

	puts("Kernel init done\n");

	bochsdbg();
	
    ExecuteFile("/files/boot/init.exe");

	IdleRoutine();
}
