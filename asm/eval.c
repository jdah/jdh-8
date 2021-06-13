#include "asm.h"

static i64 eval_exp_op(struct Context *ctx, struct Token *op, i64 lhs, i64 rhs) {
    switch (op->kind) {
        case TK_MINUS:
            if (lhs == EVAL_EXP_UNDEFINED) {
                return -rhs;
            }
            return lhs - rhs;
        case TK_NOT:
            assert(lhs == EVAL_EXP_UNDEFINED);
            return ~rhs;
        case TK_PLUS: return lhs + rhs;
        case TK_AMPERSAND: return lhs & rhs;
        case TK_PIPE: return lhs | rhs;
        case TK_STAR: return lhs * rhs;
        case TK_CARET: return lhs ^ rhs;
        case TK_SLASH: return lhs / rhs;
        case TK_LESS: return lhs << rhs;
        case TK_GREATER: return lhs >> rhs;
        default:
            asmchk(false, ctx, op, "Invalid operation");
    }

    assert(false);
    return EVAL_EXP_UNDEFINED;
}

i64 eval_exp(struct Context *ctx, struct Token *start, struct Token *end) {
    assert(start->kind == TK_LPAREN);
    assert(end->kind == TK_RPAREN);
    asmchk((usize) (start - end) > 1, ctx, start, "Empty expression");
    i64 acc = EVAL_EXP_UNDEFINED, tmp = EVAL_EXP_UNDEFINED;

    struct Token *op = NULL, *t = start->next, *u = NULL;
    while (t != end) {
        switch (t->kind) {
            case TK_NUMBER:
                tmp = t->value;
                break;
            case TK_LPAREN:
                u = t;

                usize depth = 0;
                while (u != end) {
                    if (u->kind == TK_LPAREN) {
                        depth++;
                    } else if (u->kind == TK_RPAREN) {
                        depth--;
                    }

                    if (depth == 0) {
                        break;
                    }

                    u = u->next;
                }

                asmchk(depth == 0, ctx, u, "Unbalanced expression");

                tmp = eval_exp(ctx, t, u);
                t = u;
                break;
            default:
                asmchk(!op, ctx, t, "Double operation");
                op = t;
                t = t->next;

                // do not attempt to eval, wait for rhs
                continue;
        }

        if (op) {
            acc = eval_exp_op(ctx, op, acc, tmp);
            op = NULL;
        } else {
            acc = tmp;
        }

        tmp = EVAL_EXP_UNDEFINED;
        t = t->next;
    }

    assert(acc != EVAL_EXP_UNDEFINED);
    asmchk(
        (acc < 0 && acc >= INT16_MIN) ||
        (acc >= 0 && acc <= 0xFFFF),
        ctx, start, "Expression out of bounds (value is %" PRId64 ")",
        acc
    );
    asmdbg(
        ctx, NULL,
        "Evaluated %s to %" PRIu16,
        token_between(start, end, NULL, 0), acc
    );
    return acc;
}

// evaluates expressions between the two tokens (inclusive),
// leaving other tokens alone
// returns erroneous token on error, otherwise returns NULL
struct Token *eval_between(
        struct Context *ctx,
        struct Token *start,
        struct Token *end
    ) {
    struct Token *tk = start;

    while (tk != end->next) {
        if (tk->kind != TK_LPAREN) {
            tk = tk->next;
            continue;
        }

        struct Token *end = tk;
        usize depth = 0;
        while (end->kind != TK_EOL) {
            if (end->kind == TK_LPAREN) {
                depth++;
            } else if (end->kind == TK_RPAREN) {
                depth--;
            }

            if (depth == 0) {
                break;
            }

            end = end->next;
        }

        if (depth != 0) {
            return tk;
        }

        i64 value = eval_exp(ctx, tk, end);
        tk->kind = TK_NUMBER;
        tk->value = value;

        // skip tokens in between
        tk->next = end->next;

        tk = end->next;
    }

    return NULL;
}
