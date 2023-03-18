#include "ps2kbd.h"
#include "asm.h"
#include "types.h"
#include "msglist.h"
#include "apic.h"
#include "vfs.h"
#include "kheap.h"
#include "intr.h"
#include "exec.h"
#include "proc.h"
#include "cga.h"
#include "vmem.h"

static const u8 keymap[]={
	0,k_esc,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
	'q','w','e','r','t','y','u','i','o','p','[',']','\n',k_ctrl_l,'a','s',
	'd','f','g','h','j','k','l',';','\'','`',k_shift_l,'\\','z','x','c','v',
	'b','n','m',',','.','/',k_shift_r,'*',k_alt_l,' ',k_caps,k_f1,k_f2,k_f3,k_f4,k_f5,
	k_f6,k_f7,k_f8,k_f9,k_f10,k_nums,k_scr,'7','8','9','-','4','5','6','+','1',
	'2','3','0','1',0,0,0,k_f11,k_f12
};
static const u8 keymap_s[]={
	0,k_esc,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
	'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',k_ctrl_l,'A','S',
	'D','F','G','H','J','K','L',':','\"','~',k_shift_l,'|','Z','X','C','V',
	'B','N','M','<','>','?',k_shift_r,'*',k_alt_l,' ',k_caps,k_f1,k_f2,k_f3,k_f4,k_f5,
	k_f6,k_f7,k_f8,k_f9,k_f10,k_nums,k_scr,k_home,k_up,k_pgup,'-',k_left,0,k_right,'+',k_end,
	k_down,k_pgdn,k_ins,k_del,0,0,0,k_f11,k_f12
};

int keystat;
Bool nmlk,capslk,scrlk;

void wait_kbc_ready(){
	while(inb(PORT_KEYSTA)&KEYSTA_NOREADY);
}
void wait_kbc_ack(){
	while(inb(PORT_KEYDAT)!=KBC_ACK);
}
void setled(){
	/*u8 leds=(capslk<<2)|(nmlk<<1)|scrlk;
	wait_kbc_ready();
	outb(PORT_KEYDAT,UPDATE_LED);
	wait_kbc_ack();
	wait_kbc_ready();
	outb(PORT_KEYDAT,leds);
	wait_kbc_ack();*/
    // Don't care about the lights, which brings unreasonable outputs
}

FifoBuffer kbdp;

u8 read_kbdp(){
	u8 ret=read_buffer(&kbdp);
	return ret;
}
u8 getkbdchar(){
    u8 code=read_kbdp();
    u8 ret;
    if(code<0x80){	//basic make
        if(keystat&4 || code>=0x47 && code<=0x53 && !nmlk)ret=keymap_s[code];
        else ret=keymap[code];
        if(keystat&8){
            if('a'<=ret && ret<='z')ret-=0x20;
            elif('A'<=ret && ret<='Z')ret+=0x20;
        }
        return ret;
    }
    elif(code==0xe0){	//extend
        code=read_kbdp();
        if(code<0x80){		//ext make
            if(code==0x1c)ret='\n';
            elif(code==0x1d)ret=k_ctrl_r;
            elif(code==0x35)ret='/';
            elif(code==0x38)ret=k_alt_r;
            elif(code==0x2a){
                ret=k_prtsc;
                read_kbdp();
                read_kbdp();
            }
            else ret=keymap_s[code];
            return ret;
        }
        else{		//ext break
            if(code==0x9d)return k_ctrl_rd;
            elif(code==0xb8)return k_alt_rd;
            elif(code==0xb7){
                read_kbdp();
                read_kbdp();
            }
        }
    }
    elif(code==0xe1){	//pause
        code=read_kbdp();
        read_kbdp();
        if(code<0x80)return k_pause;
        //dont care pause break
    }
    else{	//break
        if(code==0xaa)return k_shift_ld;
        elif(code==0xb6)return k_shift_rd;
        elif(code==0x9d)return k_ctrl_ld;
        elif(code==0xb8)return k_alt_ld;
    }
    return 0;
}

FifoBuffer ascp;

void commit_char(u32 cont){
    write_buffer32(&ascp, cont);
    putc(cont);
}

void kbdworkitem(){
    u8 code = getkbdchar();
    if(code == 0) return;
    if(code < 0x80){
        u32 cont = code;
        cont |= keystat<<8;
        commit_char(cont);
    }
    else{
        if(code==k_caps)keystat^=8,capslk^=1;
        elif(code==k_nums)nmlk^=1;
        elif(code==k_scr)scrlk^=1;
        else goto sctrl;
        setled();
        return;
        sctrl:
        if(code==k_ctrl_l)keystat|=0x40;
        elif(code==k_ctrl_r)keystat|=0x80;
        elif(code==k_ctrl_ld)keystat&=-0x41;
        elif(code==k_ctrl_rd)keystat&=-0x81;
        else goto salt;
        if(keystat&0xc0)keystat|=1;
        else keystat&=-2;
        return;
        salt:
        if(code==k_alt_l)keystat|=0x100;
        elif(code==k_alt_r)keystat|=0x200;
        elif(code==k_alt_ld)keystat&=-0x101;
        elif(code==k_alt_rd)keystat&=-0x201;
        else goto sshift;
        if(keystat&0x300)keystat|=2;
        else keystat&=-3;
        return;
        sshift:
        if(code==k_shift_l)keystat|=0x10;
        elif(code==k_shift_r)keystat|=0x20;
        elif(code==k_shift_ld)keystat&=-0x11;
        elif(code==k_shift_rd)keystat&=-0x21;
        else return;
        if(keystat&0x30)keystat|=4;
        else keystat&=-5;
    }
}

Export IntHandler void irq1(IntFrame* f){
	u8 x = inb(0x60);
	WriteEoi();
	write_buffer(&kbdp, x);
    Message m;
    m.arg64 = kbdworkitem;
    m.type = SPMSG_CALLBACK;
    send_message(sysprocml, &m);
}

int kbd_read(File* file, int* buf, size_t sz){
    for(int i = 0; i < sz; i++){
        int n = read_buffer32(&ascp);
        if(n == -1) return i;
        buf[i] = n;
    }
    return sz;
}
int kbd_write(File* f, int* buf, size_t sz){
    return -1;
}

void entry(int status){
    if(status == DRIVER_EXIT) return;

	// ctrl alt shift caps lshift rshift lctrl rctrl lalt ralt
	keystat=0;
	nmlk = True;
	capslk = False;
	scrlk = False;
	setled();

	kbdp.cap = 64;
    kbdp.buf = kheap_alloc(256);
    ascp.cap = 1024;
    ascp.buf = malloc_page4k_attr(1, PGATTR_NOEXEC);

    FileOperations* kbdfops = kheap_alloc_zero(sizeof(FileOperations));
    kbdfops->read = kbd_read;
    kbdfops->write = kbd_write;

    Node* n = path_walk("/run");
    n = create_subnode(n, "dev", 0);
    n = create_subnode(n, "kbd", 0);
    n->fops = kbdfops;

    IntrRedirect rte = {
		.vector = 0x21,
        .mask = False
	};
	set_gatedesc(0x21, irq1, 8, 0, 3, Interrupt);
	SetRedirect(1, rte);
}