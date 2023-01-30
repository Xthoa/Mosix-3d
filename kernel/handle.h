#pragma once

#include "types.h"
#include "spin.h"
#include "bitmap.h"

#define MAX_HANDLE 512

#define HANDLE_PROCESS 1
#define HANDLE_FILE 2
#define HANDLE_MUTEX 3
#define HANDLE_TIMER 4
#define HANDLE_HEAP 5

typedef struct s_Handle{
    void* ptr;
    u32 type;
    u32 flag;
} Handle;
typedef struct s_HandleTable{
    Handle* table;
    Bitmap* free;
    Spinlock lock;
    u32 :24;
} HandleTable;

typedef struct s_Process Process;
void alloc_htab(Process* p);
void free_htab(Process* p);

int htab_insert(HandleTable* htab, void* obj, u32 type);
void htab_delete(HandleTable* htab, int no);

int hop_openfile(char* path, int flag);
int hop_readfile(int no, char* dst, uint64_t sz);
int hop_writefile(int no, char* src, uint64_t sz);
void hop_closefile(int no);

int hop_create_process(char* path);
int hop_find_process(char* name);
void hop_wait_process(int no);
void hop_close_process(int no);

int hop_create_mutex();
void hop_close_mutex(int no);
