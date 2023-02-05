#pragma once
#pragma pack(1)

#include "stddef.h"

#define tight __attribute__((gcc_struct,packed))

#define elif else if
#define True 1
#define False 0
#define Bool _Bool

#define Status int
#define Success 0
#define ErrNull -1
#define ErrNotFound -2
#define ErrReject -3
#define ErrPermission -4
#define ErrFull -5
#define ErrUnknown -6

#define In
#define Out
#define InOpt
#define InOut
#define Optional
#define Nullable
#define NotNull
#define Notnull
#define Unnullable
#define RO
#define WO
#define RW

#define PRIVATE static
#define PUBLIC

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef long long i64;
typedef __int128 i128;
typedef unsigned __int128 u128;

typedef u8 Byte, BYTE, uchar, uint8_t;
typedef u16 Word, WORD, ushort, uint16_t;
typedef u32 Dword, DWORD, uint, uint32_t;
typedef u64 Qword, ULONGLONG, ulong, uint64_t, vaddr_t, paddr_t;

#ifdef size_t
#undef size_t
#endif

#define size_t uint64_t
#define off_t uint64_t

#define KERNEL_BASE 0xffffff8000000000
#define PAGE_SIZE 4096

#define Export __declspec(dllexport)
