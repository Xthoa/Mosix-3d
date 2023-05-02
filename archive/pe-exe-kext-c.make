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

%.exe: %.o $(LIB)
	ld $*.o -o $*.pe -e entry -s \
		--no-relax --oformat pei-x86-64 -m i386pep \
		$(LDFLAGS) $(LIB)
	objcopy $*.pe $*.exe -R .pdata -R .xdata
