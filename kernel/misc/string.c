#include "types.h"
#include <stddef.h>

void*
memmove(void *dst, const void *src, uint n)
{
  const char *s=src;
  char *d=dst;
  if(s < d && s + n > d){
    s += n;
    d += n;
    while(n-- > 0)
      *--d = *--s;
  } else
    while(n-- > 0)
      *d++ = *s++;
  return dst;
}

// memcpy exists to placate GCC.  Use memmove.
void*
memcpy(void *dst, const void *src, uint n)
{
  return memmove(dst, src, n);
}

int memcmp(const void *v1, const void *v2, uint n){
  const u8 *s1=v1, *s2=v2;
  while(n-- > 0){
	if(*s1 != *s2)return *s1 - *s2;
	s1++, s2++;
  }
  return 0;
}

char *strstr(char *str, char *sub_str){
    for(int i = 0; str[i] != '\0'; i++){
        int temp = i, j = 0;
        while(str[i++] == sub_str[j++]){
            if(sub_str[j] == '\0')return &str[temp];
        }
        i = temp;
    }
    return NULL;
}
void *memmem(u8 *str, u8 *sub_str, size_t ssz, size_t sssz,size_t bound){
    for(int i = 0; i<ssz; i+=bound){
        int temp = i, j = 0;
        while(str[i++] == sub_str[j++]){
			if(i==ssz)return NULL;
            if(j==sssz)return str+temp;
        }
        i = temp;
    }
    return NULL;
}

int strcmp(char* dst,char* src){
	while(*dst || *src){
		if(*dst!=*src)return 1;
		dst++;src++;
	}
	return 0;
}
int strncmp(char* dst,char* src,size_t n){
	if(!n)return -1;
	while((*dst || *src) && n--){
		if(*dst!=*src)return 1;
		dst++;src++;
	}
	return 0;
}
char *strcpy(char* d,char* s){
	char* dst=d;
	while(*s!=0)*(d++)=*(s++);
	*(d++)=0;
	return dst;
}
void memset(void* dst,int val,size_t sz){
	u8* d=dst;
	while(sz--)*(d++)=val;
}
char hexdig_upper(u8 i){
	if(i>=10)return i-10+'A';
	return i+'0';
}
char hexdig(u8 i){
	if(i>=10)return i-10+'a';
	return i+'0';
}

u32 hex2bcd(u32 i){
	u32 ret=0,d=8;
	u32 dig=8;
	while(dig--){
		d=i%10;
		ret+=d<<((7-dig)*4);
		i/=10;
	}
	return ret;
} 
u64 hex2bcd64(u64 i){
	u64 ret=0,d=16;
	u32 dig=16;
	while(dig--){
		d=i%10;
		ret+=d<<((15-dig)*4);
		i/=10;
	}
	return ret;
} 
u16 hex2str8(u8 i){
	return (u16)hexdig(i>>4)+((u16)hexdig(i&0xf)<<8);
}
u32 hex2str16(u16 i){
	return hex2str8(i>>8)+((u32)hex2str8(i&0xff)<<16);
}
u64 hex2str32(u32 i){
	return (u64)hex2str16(i>>16)+((u64)hex2str16(i&0xffff)<<32);
}
u128 hex2str64(u64 i){
	return (u128)hex2str32(i>>32)+((u128)hex2str32(i)<<64);
}
void hex2str(u64 i, char* dst){
	if(i == 0){
		dst[0] = '0';
		dst[1] = 0;
		return;
	}
	int d = -1;
	for(u64 x = i; x; x >>= 4) d++;
	for(u64 x = i; d >= 0; x >>= 4) dst[d--] = hexdig(x & 0xf);
	dst[d] = 0;
}

u64 min(u64 a,u64 b){
	return a<b?a:b;
}
u64 max(u64 a,u64 b){
	return a>b?a:b;
}