#include "types.h"
#include "cga.h"
#include "asm.h"
#include "stdarg.h"
#include "string.h"
#include "boot.h"
#include "spin.h"

u32 cursnow;
Spinlock cgalock;
void cga_init(){
	cursnow=0;
	init_spinlock(&cgalock);
}
void setcurspos(int pos){
	outb(CRT_ADDR,CRT_CURSPOS_HI);
	outb(CRT_DATA,pos>>8);
	outb(CRT_ADDR,CRT_CURSPOS_LO);
	outb(CRT_DATA,pos&0xff);
}
void putvram(u32 pos,u8 c,color_t color){
	u16 word = color;
	word <<= 8;
	word += c;
	VRAM[pos] = word;
}
void putc(char c){
	//acquire_spin(&cgalock);
	if(c=='\n'){
		cursnow=(cursnow+80)/80*80;
	}elif(c=='\r'){
		cursnow=(cursnow)/80*80;
	}elif(c=='\b'){
		cursnow--;
		putvram(cursnow,0,7);
	}else{
		putvram(cursnow,c,7);
		cursnow++;
	}
	if(cursnow>=80*25){
		u16 sxbit = 2 * 80;
		memmove(VRAM,VRAM+80,80*24*2);
		for(int i=80*24;i<80*25;i++){
			VRAM[i]=0;
		}
		cursnow-=80;
	}
	setcurspos(cursnow);
	//release_spin(&cgalock);
}
void puts(char* str){
	while(*str)putc(*(str++));
}

void putu8(u8 v){
	char c=hexdig((v>>4)&0xf);
	putc(c);
	c=hexdig(v&0xf);
	putc(c);
}
void putu16(u16 s){
	putu8(s>>8);
	putu8(s&0xff);
}
void putu32(u32 i){
	putu16(i>>16);
	putu16(i&0xffff);
}
void putu64(u64 l){
	putu32(l>>32);
	putu32(l&0xffffffff);
}
void putu8d(u8 val){
	putu8(hex2bcd(val));
}
void putu16d(u16 val){
	putu16(hex2bcd(val));
}
void putu32d(u32 val){
	putu32(hex2bcd(val));
}
void putu64d(u64 val){
	putu64(hex2bcd64(val));
}
void printk(char* fmt,...){
	va_list p;
	va_start(p,fmt);
	for(char c;c=*fmt;fmt++){
		if(c!='%'){
			putc(c);
			continue;
		}
		c=*(++fmt);
		if(c=='b')putu8(va_arg(p,u32));
		elif(c=='w')putu16(va_arg(p,u32));
		elif(c=='d')putu32(va_arg(p,u32));
		elif(c=='q')putu64(va_arg(p,u64));
		elif(c=='B')putu8d(va_arg(p,u32));
		elif(c=='W')putu16d(va_arg(p,u32));
		elif(c=='D')putu32d(va_arg(p,u32));
		elif(c=='Q')putu64d(va_arg(p,u64));
		elif(c=='p'){
			puts("0x");
			putu64(va_arg(p,u64));
		}
		elif(c=='c')putc(va_arg(p,int));
		elif(c=='s')puts(va_arg(p,char*));
		elif(c=='%')putc('%');
		elif(!c)break;
		else{
			putc('%');
			putc(c);
		}
	}
	va_end(p);
}