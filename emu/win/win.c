#include "win.h"
#include "mman.h"

// from https://c-for-dummies.com/blog/?p=3886
size_t strlcpy(char *restrict dst, const char *restrict src, size_t dstsize) {
    size_t offset;

    offset = 0;

    if (dstsize > 0) {
        while (*(src + offset) != '\0') {
            if (offset == dstsize) {
                offset--;
                break;
            }

            *(dst + offset) = *(src + offset);
            offset++;
        }
    }

    *(dst + offset) = '\0';

    while(*(src+offset) != '\0') {
			offset++;
		}

    return(offset);
}

// from https://github.com/Arryboom/fmemopen_windows/blob/master/libfmemopen.c

FILE *fmemopen(void *buf, size_t len, const char *type) {
	int fd;
	FILE *fp;
	char tp[MAX_PATH - 13];
	char fn[MAX_PATH + 1];
	int * pfd = &fd;
	int retner = -1;
	char tfname[] = "MemTF_";
	if (!GetTempPathA(sizeof(tp), tp))
		return NULL;
	if (!GetTempFileNameA(tp, tfname, 0, fn))
		return NULL;
	retner = _sopen_s(
        pfd, fn,
        _O_CREAT | _O_SHORT_LIVED | _O_TEMPORARY | _O_RDWR | _O_BINARY
        | _O_NOINHERIT, _SH_DENYRW, _S_IREAD | _S_IWRITE
    );
	if (retner != 0)
		return NULL;
	if (fd == -1)
		return NULL;
	fp = _fdopen(fd, "wb+");
	if (!fp) {
		_close(fd);
		return NULL;
	}
	/*File descriptors passed into _fdopen are owned by the returned FILE * stream.If _fdopen is successful, do not call _close on the file descriptor.Calling fclose on the returned FILE * also closes the file descriptor.*/
	fwrite(buf, len, 1, fp);
	rewind(fp);
	return fp;
}

char* readline(char* prompt) {
	char* retval = NULL;
	printf("> ");
	scanf("%s", retval);
	return retval;
}

void add_history(const char* string) {}
