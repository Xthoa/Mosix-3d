#pragma once
#include "types.h"

#define CRT_ADDR 0x3d4
#define CRT_DATA 0x3d5

#define CRT_BASEADR_HI 0x0c
#define CRT_BASEADR_LO 0x0d
#define CRT_CURSPOS_HI 0x0e
#define CRT_CURSPOS_LO 0x0f

#define VRAM ((u16*)(KERNEL_BASE+0x66000))

typedef u8 color_t;

extern u32 cursnow;

void InitCga();

void putc(char c);
void puts(char* str);
void putu8(u8 v);
void putu64(u64 l);
void putu8d(u8 v);
void putu64d(u64 l);