#ifndef ASM_H
#define ASM_H

#include "../common/util.h"
#include "../common/jdh8.h"

#include "lex.h"

// checks buffer _b, asmrealloc-ing it/doubling its size if necessary to add a
// new element
#define BUFCHK(_b)                                                  \
    if ((_b).buffer == NULL || (_b).size + 1 > (_b).capacity) {     \
        (_b).capacity = (_b).capacity == 0 ?                        \
            (_b).capacity_min :                                     \
            (_b).capacity * 2;                                      \
        (_b).buffer = asmrealloc(                                   \
            (_b).buffer,                                            \
            (_b).capacity * sizeof((_b).buffer[0])                  \
        );                                                          \
    }

// appends new *element _p to buffer _b, checking if it can be added first and
// creating the buffer if it does not already exist
#define BUFADDP(_b, _p) do {                                        \
        BUFCHK((_b));                                               \
        (_b).buffer[(_b).size++] = *(_p);                           \
     } while (0);

// BUFPADD, but not a pointer
#define BUFADD(_b, _e) do {                                         \
        __typeof__(_e) __e = (_e), *_p = &__e;                      \
        BUFADDP((_b), (_p));                                        \
    } while (0);

// value for Context::org indicating not set in program text
#define ORG_UNSET ((u16) 0xFFFF)

// undefined in the context of expression evaluation (see eval.c)
#define EVAL_EXP_UNDEFINED LONG_MAX

struct Input {
    char name[256];
    const char *data, *current, *line;
    usize line_no;

    struct Input *parent;
};

// token flags
enum TokenFlags {
    // indicates a symbol (label) is a sublabel
    TF_SUB = 0
};

#define A_IMM8_REG (A_IMM8 | A_REG)
#define A_IMM16_HL (A_IMM16 | A_HL | A_ADDR)
#define A_IMM (A_IMM16 | A_IMM8)
enum Arg {
    A_REG = (1 << 0),
    A_IMM8 = (1 << 1),
    A_IMM16 = (1 << 2),
    A_ADDR = (1 << 3),
    A_HL = (1 << 4)
};

struct Op {
    char name[16];

    usize num_args;
    enum Arg args[8];

    bool macro;

    union {
        // instruction
        struct {
            enum Instruction instruction;
        };

        // macro
        struct {
            char arg_names[8][16];
            struct Token *start, *end;
        };
    };
};

struct Define {
    char *name, *value;
    struct Define *next;
};

struct Label {
    char *name;
    bool sub;
    u16 value;
    struct Label *prev, *next;
};

struct Patch {
    struct Token *symbol;
    struct Label *label;
    u16 location;
    struct Patch *next;
};

// defined in microcode.c
struct MicrocodeContext;

struct Context {
    // current input
    struct Input *input;

    // always points into input->data, backed up in input->current on push
    const char *current;

    // @define'd values
    struct Define *defines;

    // labels
    struct Label *labels;

    // patches
    struct Patch *patches;

    struct {
        struct Op *buffer;
        usize size, capacity, capacity_min;
    } ops;

    // output
    struct {
        u8 *buffer;
        usize size, capacity, capacity_min;
    } out;

    // origin of output
    u16 org;

    struct MicrocodeContext *mctx;

    // flags
    struct {
        bool macro: 1;
        bool microcode: 1;
        bool verbose: 1;
    };
};

#define asmwrn(ctx, token, m, ...)\
    asmprint(                                   \
        (ctx), (token), ANSI_YELLOW "WARN",     \
        __FILE__, __LINE__, (m), ##__VA_ARGS__  \
    );

#define asmchk(e, ctx, token, m, ...) if (!(e)) {   \
        asmprint(                                   \
            (ctx), (token), ANSI_RED "ERROR",       \
            __FILE__, __LINE__, (m), ##__VA_ARGS__  \
        );                                          \
        exit(1);                                    \
    }

#define asmdbg(ctx, token, m, ...)\
    asmprint((ctx), (token), "DEBUG", __FILE__, __LINE__, (m), ##__VA_ARGS__)

void asmprint(
    const struct Context *ctx,
    const struct Token *token,
    const char *tag,
    const char *filename,
    usize line,
    const char *fmt,
    ...
);

void *asmalloc(usize n);
void *asmrealloc(void *ptr, usize n);

int push_input_buffer(struct Context *ctx, const char *name, const char *buf);
int push_input(struct Context *ctx, const char *filename);
void pop_input(struct Context *ctx);

void eval_labels(
    struct Context *ctx,
    struct Token *start,
    struct Token *end
);


// directive.c
void directive(
    struct Context *ctx,
    struct Token *first,
    bool lex
);

// eval.c
i64 eval_exp(struct Context *ctx, struct Token *start, struct Token *end);
struct Token *eval_between(
    struct Context *ctx,
    struct Token *start,
    struct Token *end
);

// macro.c
void parse_macro(
    struct Context *ctx,
    struct Token *start,
    struct Token *end
);

struct Token *expand_macro(
    struct Context *ctx,
    struct Op *op,
    struct Token *start,
    struct Token *end,
    enum Arg *args,
    struct Token **arg_tokens,
    usize num_args
);

// microcode.c
struct Token *microcode_parse(struct Context *, struct Token *);
void microcode_emit(struct Context *);

#endif
