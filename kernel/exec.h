#pragma once

#include "types.h"
#include "proc.h"

#define DEFAULT_STACKTOP 0x8000000000

Process* ExecuteFile(char* path);
