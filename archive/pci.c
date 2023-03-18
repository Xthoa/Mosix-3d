#include "pci.h"
#include "vmem.h"
#include "kheap.h"
#include "asm.h"
#include "exec.h"

volatile u32 ReadPciReg(PciAddr addr){
	outl(0xcf8,*(u32*)&addr);
	return inl(0xcfc);
}
void WritePciReg(PciAddr addr,u32 val){
	outl(0xcf8,*(u32*)&addr);
	outl(0xcfc,val);
}
void ReadPciHead(PciAddr addr,PciHead* h){
	addr.reg=0;
	for(int i=0;i<16;i++){
		addr.reg=i;
		h->regs[i]=ReadPciReg(addr);
	}
}

PciDevice* pdevroot;
Spinlock pdevllock;

void insert_device(PciDevice* dev){
    acquire_spin(&pdevllock);
    dev->next = pdevroot;
    if(pdevroot) pdevroot->prev = dev;
    pdevroot = dev;
    release_spin(&pdevllock);
}

Export void probe_devices(){
	puts("Probing PCI Devices:\n");
	puts("  Dev Fn  Vend  Devid Class\n");
	for(u8 d = 0; d < 32; d++){
		for(u8 f = 0; f < 8; f++){
			PciAddr a={
				.bus = 0,
				.dev = d,
				.func = f,
				.reg = 0,
				.enable = True
			};
            PciDevice* dev = kheap_alloc(sizeof(PciDevice));
			PciHead* h = &dev->head;
			dev->addr = a;
			ReadPciHead(a, h);
			if(h->vendor == 0xffff) break;  // nonexist device
			printk("  %b  %b  %w  %w  %b-%b-%b\n",
				a.dev, a.func, h->vendor, h->device,
				h->baseclass, h->subclass, h->progif
			);

			if(!h->multifunc) break;
		}
	}
}

void entry(int status){
    if(status == DRIVER_EXIT) return;
    pdevroot = NULL;
    init_spinlock(&pdevllock);
    //probe_devices();
}
