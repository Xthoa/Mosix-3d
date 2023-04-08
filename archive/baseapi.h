#pragma once

#include "types.h"

#define Export __declspec(dllexport)

int readfd(int fd, char* dst, size_t sz);
int writefd(int fd, char* src, size_t sz);

void* kmalloc(size_t sz);
void kfree(void* ptr);
