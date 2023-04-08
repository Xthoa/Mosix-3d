#pragma once

#include "types.h"

int tty_putchar(char c);
int tty_puts(char* s);
int tty_getchar();
int tty_getchar_wb();
int tty_gets(char* dst, size_t max);
int tty_gets_nowb(char* dst, size_t max);
int tty_printf(char* fmt, ...);
