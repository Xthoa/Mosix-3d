#pragma once

#include "types.h"

#define BootSegment KERNEL_BASE+0x48000
// #define BootArgument ((BootArguments*)(BootSegment+0x2000))

#pragma pack(1)

typedef struct s_BootArgs{
    u32 video_modes;
    u16 xsize;
    u16 ysize;
    u16 curmode;
    u16 curmode_info_off;
    u32 framebuf;
    u16 ardscnt;
    u16 ards_off;
    u32 :32;
    paddr_t smbios_phy; // unsupported
    paddr_t rsdp_phy;
    vaddr_t curmode_info;
    vaddr_t ards;
} BootArguments;


