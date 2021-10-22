#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>
#include <inttypes.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <stdarg.h>

#define ARRLEN(a) (sizeof(a) / sizeof((a)[0]))

#define MIN(_a, _b) __extension__({ \
        __typeof__(_a) __a = (_a);  \
        __typeof__(_b) __b = (_b);  \
        __a < __b ? __a : __b;      \
    })

#define MAX(_a, _b) __extension__({ \
        __typeof__(_a) __a = (_a);  \
        __typeof__(_b) __b = (_b);  \
        __a > __b ? __a : __b;      \
    })

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint64_t usize;
typedef uintptr_t uptr;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef ssize_t isize;
typedef intptr_t iptr;

#define BIT_SET(_v, _n, _x) BIT_SET_MASK(_v, (1 << (_n)), _x)

#define BIT_SET_MASK(_v, _m, _x) __extension__({        \
        __typeof__(_v) __v = (_v);                      \
        __v ^ ((-(_x) ^ __v) & (_m));                   \
    })

// true if _a prefixes _b
#define strpre(_a, _b) (strcmp((_a), (_b), strlen(_a)) == 0)

// true if _a suffixes _b
#define strsuf(_a, _b) __extension__({                  \
        const char *__a = (_a), *__b = (_b);            \
        usize _al = strlen(__a), _bl = strlen(__b);     \
        _al > _bl ?                                     \
            0 : !strncmp(__b + _bl - _al, __a, _al);    \
    })

// finds index of _s in char *_a[_n] if present (-1 if not)
#define strtab(_a, _n, _s) _strtab_impl(_a, _n, _s, strcmp)
#define strcasetab(_a, _n, _s) _strtab_impl(_a, _n, _s, strcasecmp)

#define _strtab_impl(_a, _n, _s, _cmp) __extension__({  \
        usize _i, __n = (_n);                           \
        char **__a = (char **) (_a);                    \
        for (_i = 0; _i < __n; _i++) {                  \
            if (!_cmp(__a[_i], _s)) {                   \
                break;                                  \
            }                                           \
        }                                               \
        _i == __n ? (usize) -1 : _i;                    \
    })

// converts _s to uppercase in-place
#ifndef strupr
#define strupr(_s) __extension__({                      \
        char *__s = (_s);                               \
        do {                                            \
            *__s = toupper(*__s);                       \
        } while (*(__s++) != 0);                        \
        _s;                                             \
    })
#endif

static inline char *strlstrip(char *str) {
    usize len = strlen(str);
    while (isspace(*str)) memmove(str, str + 1, --len);
    str[len] = '\0';
    return str;
}

static inline char *strrstrip(char *str) {
    usize len = strlen(str);
    while (isspace(str[len - 1])) str[--len] = '\0';
    return str;
}

static inline char *strstrip(char *str) {
    return strlstrip(strrstrip(str));
}

static inline int strtoi64(const char *s, int base, i64 *l) {
    char *t = NULL;
    errno = 0;
    i64 m = (i64) strtoll(s, &t, base);
    if (*t || (
            (m == LONG_MIN || m == LONG_MAX) && errno == ERANGE)) {
        return -1;
    }

    *l = m;
    return 0;
}

static inline int strtou64(const char *s, int base, u64 *l) {
    char *t = NULL;
    errno = 0;
    u64 m = (u64) strtoull(s, &t, base);
    if (*t || (m == ULONG_MAX && errno == ERANGE)) {
        return -1;
    }
    *l = m;
    return 0;
}

#define _DECL_STRTOX(_u, _t, _s, _min, _max)                        \
    static inline int strto##_s(const char *s, int base, _t *i) {   \
        _u##64 r;                                                   \
        int v = strto##_u##64(s, base, &r);                         \
        *i = 0;                                                     \
        if (v) {                                                    \
            return v;                                               \
        } else if (r >= (_min) && r <= (_max)) {                    \
            *i = (_t) r;                                            \
            return 0;                                               \
        }                                                           \
        return -1;                                                  \
    }

_DECL_STRTOX(i, i32, i32, INT_MIN, INT_MAX)
_DECL_STRTOX(u, u32, u32, 0, UINT_MAX)
_DECL_STRTOX(i, i16, i16, SHRT_MIN, SHRT_MAX)
_DECL_STRTOX(u, u16, u16, 0, USHRT_MAX)
_DECL_STRTOX(i, i8, i8, CHAR_MIN, CHAR_MAX)
_DECL_STRTOX(u, u8, u8, 0, INT_MAX)

static inline char escchar(char c) {
    switch (c) {
        case 'a':  c = 0x07; break;
        case 'b':  c = 0x08; break;
        case 'e':  c = 0x1B; break;
        case 'f':  c = 0x0C; break;
        case 'n':  c = 0x0A; break;
        case 'r':  c = 0x0D; break;
        case 't':  c = 0x09; break;
        case 'v':  c = 0x0B; break;
        case '\\': c = 0x5C; break;
        case '\'': c = 0x27; break;
        case '\"': c = 0x22; break;
        case '?':  c = 0x3F; break;
    }
    return c;
}

// for fancy terminal output
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_RESET   "\x1b[0m"

#define assert(_e) ( _assert((_e), false, __FILE__, __LINE__))
static inline void _assert(bool e, bool fatal, const char *file, usize line) {
    if (!e) {
        fprintf(
            stderr,
            ANSI_RED "Assertion failed. (%s:%" PRIu64 ")" ANSI_RESET "\n",
            file,
            line
        );
        exit(1);
    }
}

static inline void warn(const char *fmt, ...) {
    va_list args;
    printf("%s", ANSI_YELLOW);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("%s", ANSI_RESET);
}

#endif
