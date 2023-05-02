default:
	make -C intr
	make -C mem
	make -C misc
	make -C mp
	make -C sys
	make -C vfs
	make kernel.elf

objs = entry.o string.o kheap.o pmem.o \
		intr.o vfs.o vmem.o cpuid.o apic.o \
		timer.o except.o longjmp.o smp.o \
		apcode.o lock.o proc.o swtch.o mutex.o \
		fork.o initfs.o cga.o exec.o bitmap.o \
		handle.o msglist.o sysproc.o heap.o

kernel.elf: $(objs) kernel.lds makefile
	ld -T kernel.lds $(objs) -e KernelBootEntry -o kernel.sym --no-relax
	strip kernel.sym -o kernel.elf
	cp kernel.sym ..
	cp kernel.elf ..

.PHONY: clean
clean:
	-rm *.o kernel.elf kernel.sym
