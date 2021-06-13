#include "asm.h"

static enum Arg parse_arg_type(struct Context *ctx, struct Token *symbol) {
    char name[64];
    assert(token_data(symbol, name, sizeof(name)));

    switch (name[0]) {
        case 'r': return A_REG;
        case 'x': return A_IMM8_REG;
        case 'a': return A_IMM16 | A_ADDR;
        case 'i': return A_IMM;
        default:
            asmchk(
                false, ctx, symbol,
                "Illegal argument name (must be r, x, a, or i)"
            );
        return A_REG;
    }
}

void parse_macro(
        struct Context *ctx,
        struct Token *start,
        struct Token *end
    ) {
    asmchk(start->kind == TK_SYMBOL, ctx, start, "Expected symbol");

    struct Op op;
    memset(&op, 0, sizeof(op));
    op.macro = true;
    assert(token_data(start, op.name, sizeof(op.name)));

    isize num_addr_args = -1;

    // parse arguments
    struct Token *tk = start->next;
    while (tk->kind != TK_COLON) {
        switch (tk->kind) {
            case TK_PERCENT:
                tk = tk->next;
                asmchk(tk->kind == TK_SYMBOL, ctx, tk, "Expected symbol");
                assert(token_data(
                    tk,
                    op.arg_names[op.num_args],
                    sizeof(op.arg_names[0])
                ));
                // ensure argument name is not duplicate
                for (usize i = 0; i < op.num_args; i++) {
                    if (!strcasecmp(
                            op.arg_names[i],
                            op.arg_names[op.num_args])) {
                        asmchk(false, ctx, tk, "Duplicate argument");
                    }
                }

                op.args[op.num_args++] = parse_arg_type(ctx, tk);

                if (num_addr_args >= 0) {
                    op.args[op.num_args - 1] |= A_ADDR;
                    num_addr_args++;
                }

                break;
            case TK_LBRACKET:
                num_addr_args = 0;
                break;
            case TK_RBRACKET:
                asmchk(
                    num_addr_args > 0, ctx, tk,
                    "Must have at least one address argument"
                );
                asmchk(
                    num_addr_args < 3, ctx, tk,
                    "Too many address arguments"
                );
                asmchk(
                    num_addr_args != 1 ||
                        (op.args[op.num_args - 1] & A_ADDR) ||
                        (op.args[op.num_args - 1] & A_IMM16),
                    ctx, tk,
                    "Single address arguments must be (a)ddress or (i)mmediate"
                );
                asmchk(
                    num_addr_args != 2 ||
                        ((op.args[op.num_args - 1] & A_REG) &&
                         (op.args[op.num_args - 2] & A_REG)),
                    ctx, tk,
                    "Double address arguments must be (r)egister"
                );
                num_addr_args = -1;
                break;
            default:
                asmchk(false, ctx, tk, "Unexpected token");
        }

        tk = tk->next;
    }

    // skip TK_COLON
    asmchk(
        tk->kind == TK_COLON && tk->next->kind == TK_EOL,
        ctx, tk, "Malformed macro"
    );
    tk = tk->next;

    op.start = tk->next;
    op.end = end;

    bool empty = true;
    while (tk && tk != end) {
        if (tk->kind != TK_EOL) {
            empty = false;
            break;
        }

        tk = tk->next;
    }

    asmchk(!empty, ctx, op.start, "Empty macro");
    BUFADD(ctx->ops, op);
}


static struct Token *duplicate_macro(struct Op *op) {
    struct Token *optok = op->start,
                 *duptok = NULL,
                 *prev = NULL,
                 *result = NULL;

    while (optok != op->end->next) {
        duptok = asmalloc(sizeof(struct Token));
        memcpy(duptok, optok, sizeof(struct Token));

        duptok->prev = prev;

        if (duptok->prev) {
            duptok->prev->next = duptok;
        }

        prev = duptok;

        result = result ? result : duptok;
        optok = optok->next;
    }

    duptok->next = NULL;
    return result;
}

struct Token *expand_macro(
    struct Context *ctx,
    struct Op *op,
    struct Token *start,
    struct Token *end,
    enum Arg *args,
    struct Token **arg_tokens,
    usize num_args) {
    asmdbg(ctx, start, "Expanding macro %s", op->name);

    // duplicate macro tokens
    struct Token *tokens = duplicate_macro(op);

    // replace %<arg> with argument values
    struct Token *t = tokens, *last = tokens;
    while (t != NULL) {
        last = t;

        if (t->kind != TK_PERCENT) {
            t = t->next;
            continue;
        }

        asmchk(
            t->next && t->next->kind == TK_SYMBOL,
            ctx, t, "Expected argument name"
        );

        // skip percent
        t->prev->next = t->next;
        t = t->next;

        // replace symbol with argument
        char arg_name[256];
        assert(token_data(t, arg_name, sizeof(arg_name)));

        struct Token *argtok = NULL;

        for (usize i = 0; i < op->num_args; i++) {
            if (!strcasecmp(arg_name, op->arg_names[i])) {
                argtok = arg_tokens[i];
            }
        }

        asmchk(argtok, ctx, t, "No such argument %s\n", arg_name);

        // replace with argument by direct token copy
        struct Token *tp = t->prev, *tn = t->next;

        memcpy(t, argtok, sizeof(struct Token));
        t->prev = tp;
        t->next = tn;

        t = t->next;
    }

    // splice new macro tokens into token stream
    assert(start->prev && end->next);

    tokens->prev = start->prev;
    start->prev->next = tokens;

    last->next = end->next;
    end->next->prev = last;

    // return to parse() at the start of the new tokens
    return tokens;
}
