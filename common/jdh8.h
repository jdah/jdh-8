#ifndef JDH8_H
#define JDH8_H

#include "util.h"

#define I_NAMES ((const char *[]) {                                     \
        "MW", "LW", "SW", "PUSH", "POP", "LDA", "JNZ", "INB", "OUTB",   \
        "ADD", "ADC", "AND", "OR", "NOR", "CMP", "SBB",})
#define I_NAME(_i) (I_NAMES)[(_i)]
#define I_COUNT (I_SBB + 1)
enum Instruction {
    I_MW = 0x00,
    I_LW = 0x01,
    I_SW = 0x02,
    I_PUSH = 0x03,
    I_POP = 0x04,
    I_LDA = 0x05,
    I_JNZ = 0x06,
    I_INB = 0x07,
    I_OUTB = 0x08,
    I_ADD = 0x09,
    I_ADC = 0x0A,
    I_AND = 0x0B,
    I_OR = 0x0C,
    I_NOR = 0x0D,
    I_CMP = 0x0E,
    I_SBB = 0x0F
};

#define R_NAMES ((const char *[]){"A", "B", "C", "D", "L", "H", "Z", "F"})
#define R_NAME(_r) ((R_NAMES)[(_r)])
#define R_COUNT (R_F + 1)
enum Register {
    R_A = 0x00,
    R_B = 0x01,
    R_C = 0x02,
    R_D = 0x03,
    R_L = 0x04,
    R_H = 0x05,
    R_Z = 0x06,
    R_F = 0x07,
};

#define SP_NAMES ((const char *[]){"MB", "SP", "PC"})
#define SP_NAME(_sp) ((SP_NAMES)[(_sp)])
#define SP_COUNT (SP_PC + 1)
enum Special {
    SP_MB = 0x00,
    SP_SP = 0x01,
    SP_PC = 0x02,
};

#define F_NAMES ((const char *[]){"LESS", "EQUAL", "CARRY", "BORROW"})
#define F_NAME(_f) ((F_NAMES)[(_f)])
#define F_COUNT (F_BORROW + 1)
enum Flag {
    F_LESS      = 0x00,
    F_EQUAL     = 0x01,
    F_CARRY     = 0x02,
    F_BORROW    = 0x03,
};

enum Port {
    P_STATUS_REGISTER   = 0x00,
    P_INTERRUPT_CONTROL = 0x01
};

#define S_NAMES ((const char *[]){"INTERRUPT", "ERROR", "POWER", "HALT"})
#define S_NAME(_f) ((S_NAMES)[(_f)])
#define S_COUNT (S_HALT + 1)
enum Status {
    S_INTERRUPT = 0x00,
    S_ERROR     = 0x01,
    S_POWER     = 0x02,
    S_HALT      = 0x03
};

#define ADDR_INTERRUPT_TABLE 0xFF00
#define ADDR_MB 0xFFFA
#define ADDR_SP 0xFFFC
#define ADDR_PC 0xFFFE

#define SIZE_INTERRUPT_TABLE (ADDR_INTERRUPT_TABLE - ADDR_MB)

// forward declaration
struct JDH8;

// Limited by number of available ports - max possible.
#define DEVICE_MAX 125
struct Device {
    u8 id;
    char name[32];
    void *state;
    void (*tick)(struct JDH8 *, struct Device *);
    u8 (*send)(struct JDH8 *, struct Device *);
    void (*receive)(struct JDH8 *, struct Device *, u8 data);
    void (*destroy)(struct JDH8*, struct Device *);
};

#define FOR_DEVICES(_s, _i, _d)             \
    usize _i;                               \
    struct Device *_d;                      \
    for (_i = 0, _d = &(_s)->devices[_i];   \
            _i < DEVICE_MAX;                \
            _i++,_d = &(_s)->devices[_i])   \

// maximum number of memory banks
#define BANKS_MAX 256

struct JDH8 {
    u8 status;

    union {
        struct {
            u16 mb, sp, pc;
        };

        u16 raw[3];
    } special;

    union {
        struct {
            u8 rom[0x8000];
            u8 bank[0x4000];
            u8 ram[0x4000];
        };

        u8 raw[0xFFFF];
    } memory;

    // non-NULL if present
    void *banks[BANKS_MAX];

    union {
        struct {
            u8 a, b, c, d, l, h, z, f;
        };

        struct {
            u16 __ignore0[2];
            u16 hl;
            u16 __ignore1;
        };

        u8 raw[8];
    } registers;

    struct Device devices[DEVICE_MAX];
    bool simulating;
};

static inline u8 *ppeek(struct JDH8 *state, u16 addr) {
    if (addr >= ADDR_MB) {
        return &((u8 *) state->special.raw)[addr - ADDR_MB];
    } else if (state->special.mb != 0 && addr >= 0x8000 && addr < 0xC000) {
        void *bank = state->banks[state->special.mb];

        if (!bank) {
            warn(
                "Read/write attempt for non-present bank 0x%04x\n",
                state->special.mb
            );
            return NULL;
        }

        return &((u8 *) bank)[addr - 0x8000];
    }

    return &state->memory.raw[addr];
}

static inline u8 peek(struct JDH8 *state, u16 addr) {
    u8 *p = ppeek(state, addr);
    return p ? *p : 0;
}

static inline u16 peek16(struct JDH8 *state, u16 addr) {
    return
        (((u16) (peek(state, addr + 0))) << 0) |
        (((u16) (peek(state, addr + 1))) << 8);
}

static inline void poke(struct JDH8 *state, u16 addr, u8 v) {
    u8 *p = ppeek(state, addr);
    if (p) {
        *p = v;
    }
}

static inline void poke16(struct JDH8 *state, u16 addr, u16 v) {
    poke(state, addr + 0, (v >> 0) & 0xFF);
    poke(state, addr + 1, (v >> 8) & 0xFF);
}

static inline void emu_memcpy(
        struct JDH8 *state,
        u16 dest,
        const void *src,
        usize n
    ) {
    usize i = 0;
    while (i != n) {
        poke(state, dest + i, ((const u8 *) src)[i]);
        i++;
    }
}

static inline void emu_memset(
        struct JDH8 *state,
        u16 dest,
        u8 data,
        usize n
    ) {
    while (n-- != 0) {
        poke(state, dest++, data);
    }
}

#endif
