#pragma once

#include "types.h"

typedef struct s_Partition{
    u8 attr;
    u8 chs[3];
    u8 type;
    u8 chsend[3];
    u32 lba;
    u32 secs;
} Partition;

typedef struct s_Mbr{
    u8 code[440];
    u32 diskid;
    u16 ro;
    Partition part[4];
    u16 magic;
} Mbr;
