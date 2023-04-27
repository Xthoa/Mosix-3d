#pragma once

#include "types.h"
#include "vfs.h"

#define ARCHIVE_ADDR ((TarMetadata*)(KERNEL_BASE + 0x20000))

#pragma pack(1)
typedef struct s_TarMetadata{
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    u8 type;
    char linkname[100];
    char ustar[6];
    char tarver[2];
    char uname[32];
    char gname[32];
    u64 devmajor;
    u64 devminor;
    char fnprefix[155];
    u32 isize;  // converted size (software use)
    u8 cflag;
    u8 pad[7];
} TarMetadata;

