#include "jdh8util.h"
#include "jdh8.h"

// reverse assembles some bytes
// returns non-zero on failure
int asm2str(char *dst, usize dst_size, u8 *data, usize n) {
#define __asm2str_append(_s) do {               \
        const char *__s = (_s);                 \
        usize _sz = strlen(__s);                \
        if (p + _sz >= dst + dst_size) {        \
            return -1;                          \
        }                                       \
        memcpy(p, __s, _sz);                    \
        p += _sz;                               \
    } while (0);

#define __asm2str_expect_size(_n) if (n <= (_n)) { return -1; }

#define __asm2str_append_addr() do {                    \
        __asm2str_expect_size(3);                       \
        addr = *((u16 *) (data + 1));                   \
        snprintf(buf, sizeof(buf), "0x%04x", addr);     \
        __asm2str_append(buf);                          \
    } while (0);

#define __asm2str_illegal() \
    do { p = dst; __asm2str_append("ILLEGAL INSTRUCTION"); } while (0);

    if (dst_size < 1) {
        return -1;
    }

    char buf[256], *p = dst;

    u16 addr;
    u8 op0 = data[0], op1,
       inst = (op0 >> 4) & 0xF,
       y = (op0 >> 3) & 1,
       reg = op0 & 0x7;

    // write instruction name
    __asm2str_append(I_NAMES[inst]);
    __asm2str_append(" ");

    // reg, imm8/reg
    if (inst == I_MW || inst == I_INB || inst >= I_ADD) {
        __asm2str_expect_size(2);
        op1 = data[1];

        __asm2str_append(R_NAMES[reg]);
        __asm2str_append(", ");

        if (y) {
            __asm2str_append(R_NAMES[op1]);
        } else {
            snprintf(buf, sizeof(buf), "0x%02x", op1);
            __asm2str_append(buf);
        }
    } else if (inst == I_LW) {
        __asm2str_append(R_NAMES[reg]);

        if (!y) {
            __asm2str_append(", ");
            __asm2str_append_addr();
        }
    } else if (inst == I_SW) {
        if (!y) {
            __asm2str_append_addr();
            __asm2str_append(", ");
        }

        __asm2str_append(R_NAMES[reg]);
    } else if (inst == I_PUSH || inst == I_JNZ) {
        if (y) {
            __asm2str_append(R_NAMES[reg]);
        } else {
            __asm2str_expect_size(2);
            op1 = data[1];
            snprintf(buf, sizeof(buf), "0x%02x", op1);
            __asm2str_append(buf);
        }
    } else if (inst == I_POP) {
        if (y) {
            __asm2str_append(R_NAMES[reg]);
        } else {
            __asm2str_illegal();
        }
    } else if (inst == I_LDA) {
        __asm2str_append_addr();
    } else if (inst == I_OUTB) {
        __asm2str_expect_size(2);
        op1 = data[1];

        if (y) {
            __asm2str_append(R_NAMES[op1 & 0x7]);
            __asm2str_append(", ");
        } else {
            snprintf(buf, sizeof(buf), "0x%02x", op1);
            __asm2str_append(buf);
            __asm2str_append(", ");
        }

        __asm2str_append(R_NAMES[reg]);
    }

    *p = '\0';
    return 0;
}
