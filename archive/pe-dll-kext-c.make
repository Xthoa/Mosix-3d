CC = x86_64-w64-mingw32-gcc
CFLAGS = -w -O1 -m64 -fPIC -shared \
	-mabi=sysv \
	-I../kernel \
	-mno-red-zone \
	-mmanual-endbr \
	-mgeneral-regs-only \
	-fno-stack-protector \
	-fcf-protection=none \
	-fno-asynchronous-unwind-tables

%.dll: %.o $(LIB)
	ld $*.o -o $*.dll -s -e dlentry \
		--no-relax --oformat pei-x86-64 -m i386pep \
		--shared $(LDFLAGS) $(LIB)
	objcopy $*.dll -R .pdata -R .xdata
