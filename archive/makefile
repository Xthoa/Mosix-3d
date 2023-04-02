default: archive

arch = test.txt init.exe test.dll pci.drv ps2kbd.drv idedisk.drv

ckexe = make -f pe-exe-kern-c.make
ckdll = make -f pe-dll-kern-c.make
ckdrv = make -f pe-drv-kern-c.make

archive:
	$(ckdll) test.dll
	$(ckexe) init.exe LIB=test.dll
	$(ckdrv) pci.drv
	$(ckdrv) ps2kbd.drv
	$(ckdrv) idedisk.drv
	tar -cf archive.tar $(arch)
	cp archive.tar ..

.PHONY: clean
clean:
	-rm archive.tar *.o *.pe *.elf
