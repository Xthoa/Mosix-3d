CC = x86_64-w64-mingw32-gcc
CFLAGS = -w -O1 -m64 -fPIC \
	-mabi=sysv \
	-I../kernel \
	-mno-red-zone \
	-mmanual-endbr \
	-mcmodel=large \
	-mgeneral-regs-only \
	-fno-stack-protector \
	-fcf-protection=none \
	-fno-asynchronous-unwind-tables

%.dll: %.o ../kernel.elf $(LIB)
	ld $*.o -R ../kernel.elf -o $*.dll -s -e dlentry \
		--no-relax --oformat pei-x86-64 -m i386pep \
		--shared $(LDFLAGS) $(LIB)
	objcopy $*.dll -R .pdata -R .xdata
