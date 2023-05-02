CC = x86_64-w64-mingw32-gcc
CFLAGS = -w -O1 -m64 \
	-mabi=sysv \
	-I../kernel/include \
	-mno-red-zone \
	-mmanual-endbr \
	-mcmodel=large \
	-mgeneral-regs-only \
	-fno-stack-protector \
	-fcf-protection=none \
	-fno-asynchronous-unwind-tables

%.drv: %.o ../kernel.sym $(LIB)
	ld $*.o -R ../kernel.sym -o $*.pe -s -e entry --subsystem=35 \
		--no-relax --oformat pei-x86-64 -m i386pep --shared \
		$(LDFLAGS) $(LIB)
	objcopy $*.pe $*.drv -R .pdata -R .xdata
