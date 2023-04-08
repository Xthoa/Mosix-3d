#pragma once

#include "stdarg.h"
#include "types.h"

int sprintf(char *buf, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);
