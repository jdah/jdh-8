#ifndef _WIN_H_
#define _WIN_H_

#include <stdio.h>
#include <windows.h>
#include <share.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

size_t strlcpy(char *restrict dst, const char *restrict src, size_t dstsize);

FILE *fmemopen(void *buf, size_t len, const char *type);

char* readline(char* prompt);
void add_history(const char* string);

#endif
