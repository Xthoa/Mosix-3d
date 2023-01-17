default: archive

arch = test.txt init.exe

ckexe = make -f pe-exe-kern-c.make

archive:
	$(ckexe) init.exe
	tar -cf archive.tar $(arch)
	cp archive.tar ..

.PHONY: clean
clean:
	-rm archive.tar *.o *.pe *.elf
