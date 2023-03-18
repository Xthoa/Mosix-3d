#pragma once

#include "types.h"
#include "vfs.h"
#include "spin.h"

#define DEFAULT_STACKTOP 0x8000000000

#define PEDLL_BOTTOM 0x7f8000000000
#define PEDLL_TOP 0x800000000000
#define DRIVER_BOTTOM 0xffffff8400000000
#define DRIVER_MAX 0x100000000

#define DRIVER_INIT 0
#define DRIVER_EXIT 1

#define SPMSG_CALLBACK 2

typedef struct s_Pesection{
    paddr_t paddr;  // shared data if shareable; proto data if not.
    vaddr_t voff;
    u32 pages;  // 4k page count of total size
    u32 size;   // data size
    u16 flags;
    u16 bsssz;  // bss size (fill 0 after data)
} Pesection;
typedef struct s_Pedll{
    struct s_Pedll* next;
    struct s_Pedll* prev;
    Pesection* sec;
    char* name;
    Node* actual;
    u32 entry;
    u32 imgsize;
    u32 edata;
    u32 idata;
    u32 relocs;
    u32 relocsz;
    u64 relocfix;
    u64 base;
    u16 seccnt;
} Pedll;

typedef struct s_ActivePedll{
    Pedll* dll;
    u64 base;
} ActivePedll;
typedef struct s_PeInfo{
    ActivePedll* dlls;
    u64 base;
    u32 idata;
    u16 count;
    Spinlock lock;
} PeInfo;

typedef struct s_Process Process;

Process* ExecuteFile(char* path);
Pedll* LoadDriver(char* name);
void UnloadDriver(Pedll* dll);
