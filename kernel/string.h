#pragma once

#include "types.h"

int memcmp(const void *v1, const void *v2, uint n);
void *memmem(u8 *str, u8 *sub_str, size_t ssz, size_t sssz,size_t bound);

void memset(void* dst,int val,size_t sz);

u64 hex2bcd64(u64 i);
u32 hex2bcd(u32 i);

u16 hex2str8(u8 i);
u32 hex2str16(u16 i);
u64 hex2str32(u32 i);
u128 hex2str64(u64 i);

u64 min(u64 a,u64 b);
u64 max(u64 a,u64 b);
