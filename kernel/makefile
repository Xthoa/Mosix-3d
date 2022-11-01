default: kernel.elf

CFLAGS = -w -O1 -m64 \
	-mno-red-zone \
	-mmanual-endbr \
	-mgeneral-regs-only \
	-fno-stack-protector \
	-fcf-protection=none \
	-fno-asynchronous-unwind-tables

objs = entry.o string.o kheap.o pmem.o \
		intr.o vfs.o vmem.o cpuid.o apic.o \
		timer.o except.o longjmp.o smp.o \
		apcode.o lock.o proc.o swtch.o mutex.o \
		fork.o initfs.o cga.o

%.o: %.asm makefile
	nasm -f elf64 -o $*.o $*.asm

kernel.elf: $(objs) kernel.lds makefile
	ld -T kernel.lds $(objs) -e KernelBootEntry -o kernel.elf --no-relax
	cp kernel.elf ..

.PHONY: clean
clean:
	-rm *.o kernel.elf