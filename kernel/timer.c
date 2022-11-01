#include "timer.h"
#include "intr.h"
#include "apic.h"
#include "asm.h"

PUBLIC volatile uint64_t jiffies;
IntHandler void irq2_handler(IntFrame* f){
    jiffies ++;
    WriteEoi();
    IpiCommand ipi = {
        .vector = 0x38,
        .delivmod = Fixed,
    };
    SendIpi(-AbbrOthers, ipi);
    // sched_tick();
}
void timer_init(){
    // enable PIT and set to periodic; 1kHz (factor = 0x4a9)
	outb(0x43,0x34);
	outb(0x40,0xa9);
	outb(0x40,0x04);

    // set the IRQ2 redirector and handler
	set_gatedesc(0x22, irq2_handler, 16, 0, 3, Interrupt);
    IntrRedirect rte = {
        .vector = 0x22,
        .mask = False
    };
	SetRedirect(2, rte);

    jiffies = 0;
}