#ifndef _WIN_H_
#define _WIN_H_

#include <stdio.h>
#include <windows.h>
#include <share.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

size_t strlcpy(char *restrict dst, const char *restrict src, size_t dstsize);

#define POLLIN 0x0001
#define POLLPRI 0x0002 /* not used */
#define POLLOUT 0x0004
#define POLLERR 0x0008
#define POLLHUP 0x0010 /* not used */
#define POLLNVAL 0x0020 /* not used */

struct pollfd {
	int fd;
	int events; /* in param: what to poll for */
	int revents; /* out param: what events occured */
};
int poll (struct pollfd *p, int num, int timeout);

FILE *fmemopen(void *buf, size_t len, const char *type);

char* readline(char* prompt);
void add_history(const char* string);

#endif
