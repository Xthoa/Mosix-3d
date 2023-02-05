default: build

.PHONY: archive
archive:
	$(MAKE) -C archive

.PHONY: kernel
kernel:
	$(MAKE) -C kernel

.PHONY: boot
boot:
	$(MAKE) -C boot

.PHONY: build
build:
	$(MAKE) boot kernel archive
	$(DD) if=boot.bin of=mosix.img bs=512 seek=0 count=4
	$(DD) if=kernel.elf of=mosix.img bs=512 seek=4
	$(DD) if=archive.tar of=mosix.img bs=512 seek=84
	truncate mosix.img --size=%512

.PHONY: clean
clean:
	$(MAKE) -C boot clean
	$(MAKE) -C kernel clean
	$(MAKE) -C archive clean
	-rm boot.bin kernel.elf archive.tar mosix.img


SMP := ENABLED

DD ?= dd
BOCHS ?= bochs
BOCHS_DEBUG ?= bochsdbg
QEMU ?= qemu-system-x86_64
QEMU_ARGS ?= -fda mosix.img \
			 -m 32 \
			 -rtc base=localtime \
			 -name "Mosix 3c" 
QEMU_DEBUG_ARGS ?= $(QEMU_ARGS) -s -S
ifeq ($(SMP), ENABLED)
	BOCHS_ARGS += -f bochsrcmp.bxrc
	QEMU_ARGS += -smp 2
else
	BOCHS_ARGS += -f bochsrc.bxrc
endif

.PHONY: bochs
bochs:
	$(BOCHS) $(BOCHSARGS)

.PHONY: bochs-debug
bochs-debug:
	$(BOCHSDBG) $(BOCHSARGS)

.PHONY: qemu
qemu:
	$(QEMU) $(QEMU_ARGS)

.PHONY: qemu-debug
qemu-debug:
	$(QEMU) $(QEMU_DEBUG_ARGS)
