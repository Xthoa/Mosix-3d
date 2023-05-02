#pragma once

#include "types.h"

typedef struct s_Cpuid{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
} Cpuid;

void GetCpuid(uint32_t* data,uint32_t eax);