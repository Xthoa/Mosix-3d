default: boot.bin

boot.bin: boot.s makefile
	nasm boot.s -o boot.bin
	cp boot.bin ..

.PHONY: clean
clean:
	-rm boot.bin