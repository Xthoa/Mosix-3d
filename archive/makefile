default: archive

arch = test.txt init.exe ttyio.dll pci.drv \
	ps2kbd.drv idedisk.drv pty.drv vgatty.exe \
	sh0.exe sprintf.dll mem.exe baseapi.dll \
	pwd.exe

ckexe = make -f pe-exe-kern-c.make
ckdll = make -f pe-dll-kern-c.make
ckdrv = make -f pe-drv-kern-c.make
ckedll = make -f pe-dll-kext-c.make
ckeexe = make -f pe-exe-kext-c.make

archive:
	$(ckdrv) pci.drv
	$(ckdrv) ps2kbd.drv
	$(ckdrv) idedisk.drv
	$(ckdrv) pty.drv
	$(ckdll) baseapi.dll
	$(ckedll) sprintf.dll
	$(ckedll) ttyio.dll LIB="sprintf.dll baseapi.dll"
	$(ckexe) init.exe
	$(ckexe) vgatty.exe
	$(ckexe) mem.exe LIB=ttyio.dll
	$(ckexe) sh0.exe LIB=ttyio.dll
	$(ckexe) pwd.exe LIB=ttyio.dll
	tar -cf archive.tar $(arch)
	cp archive.tar ..

.PHONY: clean
clean:
	-rm archive.tar *.o *.pe *.elf
