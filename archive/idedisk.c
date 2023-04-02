#include "types.h"
#include "vfs.h"
#include "asm.h"
#include "except.h"
#include "exec.h"
#include "macros.h"
#include "mutex.h"
#include "kheap.h"

const u32 ports[]={
	0x1f0,
	0x170
};
Bool exist[4];

void wait_ide_io(u32 no){
	inb(ports[no>>1]+12);
	inb(ports[no>>1]+12);
	inb(ports[no>>1]+12);
	inb(ports[no>>1]+12);
}

Bool test_existance(u32 no){
	outb(ports[no>>1]+2,0);
	outb(ports[no>>1]+3,0);
	outb(ports[no>>1]+4,0);
	outb(ports[no>>1]+5,0);
	outb(ports[no>>1]+6,0xa0|((no&1)<<4));
    outb(ports[no>>1]+7,0xec);
	wait_ide_io(no);
    while((inb(ports[no>>1]+7) & 0x80) != 0);
	u8 cl = inb(ports[no>>1]+4);
	u8 ch = inb(ports[no>>1]+5);
	if(cl == 0xff && ch == 0xff) return False;
	if((cl == 0 && ch == 0) || (cl == 0x3c && ch == 0xc3)){
		return True;
	}
	return False;
}

void wait_ide(u32 no){
	ASSERT(no < 4);
    ASSERT(exist[no]);
	while((inb(ports[no>>1]+7)&0x80)!=0);
}
void write_addr28(u32 no,u8 cnt,u32 off){
	wait_ide(no);
	outb(ports[no>>1]+2,cnt);
	outb(ports[no>>1]+3,off);
	outb(ports[no>>1]+4,off>>8);
	outb(ports[no>>1]+5,off>>16);
	outb(ports[no>>1]+6,off>>24|0xe0|((no&1)<<4));
}
void write_addr48(u32 no,u16 cnt,u64 off){
	wait_ide(no);
	outb(ports[no>>1]+8,cnt>>8);
	outb(ports[no>>1]+9,off>>24);
	outb(ports[no>>1]+10,off>>32);
	outb(ports[no>>1]+11,off>>40);
	outb(ports[no>>1]+2,cnt);
	outb(ports[no>>1]+3,off);
	outb(ports[no>>1]+4,off>>8);
	outb(ports[no>>1]+5,off>>16);
	outb(ports[no>>1]+6,0xe0|((no&1)<<4));
}
void read_sect28(u32 no,char* dst,u32 off){
	write_addr28(no,1,off);
	outb(ports[no>>1]+7,0x20);
	wait_ide(no);
	insl(ports[no>>1],dst,128);
}
void read_sects28(u32 no,char* dst,u32 off,u8 len){
	write_addr28(no,len,off);
	outb(ports[no>>1]+7,0x20);
	wait_ide(no);
	insl(ports[no>>1],dst,128*len);
}
void read_sect48(u32 no,char* dst,u64 off){
	write_addr48(no,1,off);
	outb(ports[no>>1]+7,0x24);
	wait_ide(no);
	insl(ports[no>>1],dst,128);
}
void read_sects48(u32 no,char* dst,u64 off,u16 len){
	write_addr48(no,len,off);
	outb(ports[no>>1]+7,0x24);
	wait_ide(no);
	insl(ports[no>>1],dst,128*len);
}
void write_sect28(u32 no,char* src,u32 off){
	write_addr28(no,1,off);
	outb(ports[no>>1]+7,0x30);
	wait_ide(no);
	outsl(ports[no>>1],src,128);
}
void write_sects28(u32 no,char* src,u32 off,u8 len){
	write_addr28(no,len,off);
	outb(ports[no>>1]+7,0x30);
	wait_ide(no);
	outsl(ports[no>>1],src,128);
}
void write_sect48(u32 no,char* src,u64 off){
	write_addr48(no,1,off);
	outb(ports[no>>1]+7,0x34);
	wait_ide(no);
	outsl(ports[no>>1],src,128);
}
void write_sects48(u32 no,char* src,u64 off,u16 len){
	write_addr48(no,len,off);
	outb(ports[no>>1]+7,0x34);
	wait_ide(no);
	outsl(ports[no>>1],src,128);
}

Mutex* dlock[4];

int ide_open(Node* inode, File* file){
    file->data = inode->data;
    return 0;
}
int ide_read(File* file, char* buf, size_t size){
    int i = file->data;
    acquire_mutex(dlock[i]);
    if((file->off >> 28) || (size >> 8)){
        read_sects48(i, buf, file->off, size);
    }
    else read_sects28(i, buf, file->off, size);
    release_mutex(dlock[i]);
    file->off += size;
    return size;
}
int ide_write(File* file, char* buf, size_t size){
    int i = file->data;
    acquire_mutex(dlock[i]);
    if((file->off >> 28) || (size >> 8)){
        write_sects48(i, buf, file->off, size);
    }
    else write_sects28(i, buf, file->off, size);
    release_mutex(dlock[i]);
    file->off += size;
    return size;
}

void entry(int status){
    if(status == DRIVER_EXIT) return;

    u8 st = inb(0x1f7);
    if(st == 0xff) exist[0] = exist[1] = False;
    else {
        exist[0] = test_existance(0);
        exist[1] = test_existance(1);
    }
    st = inb(0x177);
    if(st == 0xff) exist[2] = exist[3] = False;
    else {
        exist[2] = test_existance(2);
        exist[3] = test_existance(3);
    }

    FileOperations* idefops = kheap_alloc_zero(sizeof(FileOperations));
    idefops->open = ide_open;
    idefops->read = ide_read;
    idefops->write = ide_write;

	Node* dev = path_walk("/run/dev");
    char* name = "ide0";
    for(int i = 0; i < 4; i++){
		if(exist[i] == False) continue;
        Node* n = create_subnode(dev, name, 0);
        n->data = i;
        n->fops = idefops;
        dlock[i] = kheap_alloc(sizeof(Mutex));
        init_mutex(dlock[i]);
        name[3] ++;
    }
}