#include "asm.h"

#define MOP(_n, _t, _b)\
    [(_t * 16) + _b] = ((struct MicroOp) { #_n, true, _t, _b })
struct MicroOp {
    const char *name;
    bool present;
    u8 type, index;
};

// microcode types
#define MTA 0
#define MTB 1
#define MTG 2
#define MTD 4

struct MicroOp MICRO_OPS[] = {
    // TYPE A MICROCODE
    MOP(ERAM, MTA, 0),
    MOP(ASP, MTA, 1),
    MOP(AIMM, MTA, 2),
    MOP(LRAM, MTA, 3),
    MOP(SPINC, MTA, 4),
    MOP(SPDEC, MTA, 5),
    MOP(JNZ, MTA, 6),

    // TYPE B MICROCODE
    MOP(LDX, MTB, 0),
    MOP(LDY, MTB, 1),
    MOP(LDF, MTB, 2),
    MOP(EALU, MTB, 3),
    MOP(LDH, MTB, 4),
    MOP(LDL, MTB, 5),
    MOP(LPRT, MTB, 6),

    // GENERIC MICROCODE
    MOP(EREG, MTG, 7),
    MOP(LREG, MTG, 8),
    MOP(SEL, MTG, 9),
    MOP(EOP1, MTG, 10),
    MOP(EOP2, MTG, 11),
    MOP(EDEV, MTG, 12),
    MOP(LDEV, MTG, 13),
    MOP(CC, MTG, 14),

    // DUMMY MICROCODE
    MOP(AHL, MTD, 0),
};

struct MicrocodeContext {
    // instructions * const/reg * microcode entries
    u16 code[I_COUNT][2][8];

    enum Instruction instruction;
    bool reg;
    u8 index;
};

const struct MicroOp *find_op(const char *name) {
    const struct MicroOp *op = NULL;

    for (usize i = 0; i < ARRLEN(MICRO_OPS); i++) {
        if (MICRO_OPS[i].present &&
                !strcasecmp(name, MICRO_OPS[i].name)) {
            op = &MICRO_OPS[i];
            break;
        }
    }

    return op;
}

struct Token *microcode_parse(struct Context *ctx, struct Token *first) {
    if (!ctx->mctx) {
        ctx->mctx = asmalloc(sizeof(struct MicrocodeContext));
        memset(ctx->mctx, 0, sizeof(struct MicrocodeContext));
    }

    struct Token *last, *eol = first;
    while (eol->kind != TK_EOL) {
        eol = eol->next;
    }
    last = eol->prev;

    // label
    if (last->kind == TK_COLON) {
        // must be .const or .reg
        if (first->kind == TK_DOT) {
            asmchk(
                first->next->kind == TK_SYMBOL,
                ctx, first->next,
                "Expected symbol"
            );

            char symbol[256];
            assert(token_data(first->next, symbol, sizeof(symbol)));

            if (!strcasecmp(symbol, "const")) {
                ctx->mctx->reg = false;
            } else if (!strcasecmp(symbol, "reg")) {
                ctx->mctx->reg = true;
            } else {
                asmchk(false, ctx, first->next, "Illegal microcode label");
            }

            ctx->mctx->index = 0;
        } else {
            asmchk(
                first->kind == TK_SYMBOL,
                ctx, first,
                "Expected symbol"
            );

            char symbol[256];
            assert(token_data(first, symbol, sizeof(symbol)));

            isize i;
            asmchk(
                (i = strcasetab(I_NAMES, I_COUNT, symbol)) != -1,
                ctx, first,
                "Unknown instruction for microcode"
            );
            asmdbg(ctx, first, "Parsing microcode for %s", symbol);

            ctx->mctx->instruction = (enum Instruction) i;
            ctx->mctx->reg = false;
            ctx->mctx->index = 0;
        }

        return eol->next;
    }

    asmchk(
        first->kind == TK_SYMBOL || first->kind == TK_NOT,
        ctx, first,
        "Expected symbol or ~"
    );

    struct Token *t = first;

    const struct MicroOp *op;
    char symbol[256];

    u16 cval = 0;
    u8 ctype = MTG;

    // check if macro
    if (t->kind == TK_SYMBOL && t->next == eol) {
        assert(token_data(t, symbol, sizeof(symbol)));

        struct Op *op = &ctx->ops.buffer[0];
        while (op != &ctx->ops.buffer[ctx->ops.size]) {
            if (!op->macro || strcasecmp(symbol, op->name)) {
                op++;
                continue;
            }

            return expand_macro(ctx, op, first, eol->prev, NULL, NULL, 0);
        }
    }

    while (t != eol) {
        switch (t->kind) {
            case TK_NOT:
                asmchk(t->prev->kind != TK_NOT, ctx, t->prev, "Too many ~");
                t = t->next;
                continue;
            case TK_SYMBOL:
                assert(token_data(t, symbol, sizeof(symbol)));
                asmchk(op = find_op(symbol), ctx, t, "Unknown op");

                // skip dummy operations
                if (op->type == MTD || t->prev->kind == TK_NOT) {
                    t = t->next;
                    continue;
                }

                asmchk(
                    ctype == MTG || op->type == MTG || op->type == ctype,
                    ctx, t,
                    "Cannot combine ops of different types"
                );
                ctype = op->type != MTG ? op->type : ctype;
                cval |= 1 << op->index;
                break;
            default:
                asmchk(false, ctx, t, "Expected ~ or symbol");
        }

        t = t->next;
    }

    cval = BIT_SET(cval, 15, ctype == MTB ? 1 : 0);
    ctx->mctx->code
        [ctx->mctx->instruction]
        [ctx->mctx->reg ? 1 : 0]
        [ctx->mctx->index++] = cval;

    return eol->next;
}

void microcode_emit(struct Context *ctx) {
    for (enum Instruction i = 0; i < I_COUNT; i++) {
        for (u8 reg = 0; reg < 2; reg++) {
            const u16 *imc = ctx->mctx->code[i][reg];

            for (u8 j = 0; j < 8; j++) {
                u16 data = imc[j];

                if (j == 7 || imc[j + 1] == 0) {
                    data |= 1 << 14;
                }

                BUFADD(ctx->out, ((data >> 0) & 0xFF));
                BUFADD(ctx->out, ((data >> 8) & 0xFF));
            }
        }
    }
}
