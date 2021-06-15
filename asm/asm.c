#include "asm.h"
#include "lex.h"

// from builtin_macros.c, produced by builtin_macros.asm
extern unsigned char asm_macros_asm[];
extern unsigned int asm_macros_asm_len;

#define BUILTIN_MACROS_TEXT (asm_macros_asm)
#define BUILTIN_MACROS_LEN  (asm_macros_asm_len)

// program-lifetime memory region
#define REGION_MIN_SIZE 0x40000
struct Region {
    void *current, *end, *next;
};

static const struct Op INSTRUCTIONS[] = {
    [I_MW] = {
        .name = "MW",
        .num_args = 2,
        .args = { A_REG, A_IMM8_REG },
        .macro = false,
        .instruction = I_MW
    },
    [I_LW] = {
        .name = "LW",
        .num_args = 2,
        .args = { A_REG, A_IMM16_HL },
        .macro = false,
        .instruction = I_LW
    },
    [I_SW] = {
        .name = "SW",
        .num_args = 2,
        .args = { A_IMM16_HL, A_REG },
        .macro = false,
        .instruction = I_SW
    },
    [I_PUSH] = {
        .name = "PUSH",
        .num_args = 1,
        .args = { A_IMM8_REG },
        .macro = false,
        .instruction = I_PUSH
    },
    [I_POP] = {
        .name = "POP",
        .num_args = 1,
        .args = { A_REG },
        .macro = false,
        .instruction = I_POP
    },
    [I_LDA] = {
        .name = "LDA",
        .num_args = 1,
        .args = { A_IMM16 },
        .macro = false,
        .instruction = I_LDA
    },
    [I_JNZ] = {
        .name = "JNZ",
        .num_args = 1,
        .args = { A_IMM8_REG },
        .macro = false,
        .instruction = I_JNZ
    },
    [I_INB] = {
        .name = "INB",
        .num_args = 2,
        .args = { A_REG, A_IMM8_REG },
        .macro = false,
        .instruction = I_INB
    },
    [I_OUTB] = {
        .name = "OUTB",
        .num_args = 2,
        .args = { A_IMM8_REG, A_REG },
        .macro = false,
        .instruction = I_OUTB
    },

#define DEF_OP_ARITHMETIC(_n)           \
    [I_##_n] = {                        \
        .name = #_n,                    \
        .num_args = 2,                  \
        .args = { A_REG, A_IMM8_REG },  \
        .macro = false,                 \
        .instruction = I_##_n           \
    }

    DEF_OP_ARITHMETIC(ADD),
    DEF_OP_ARITHMETIC(ADC),
    DEF_OP_ARITHMETIC(AND),
    DEF_OP_ARITHMETIC(OR),
    DEF_OP_ARITHMETIC(NOR),
    DEF_OP_ARITHMETIC(CMP),
    DEF_OP_ARITHMETIC(SBB),
};

// head of memory region linked list
static struct Region *region = NULL;

void asmprint(
        const struct Context *ctx,
        const struct Token *token,
        const char *tag,
        const char *filename,
        usize line,
        const char *fmt,
        ...) {
    va_list args;
    va_start(args, fmt);

    if ((!ctx || !ctx->verbose) && !strcmp(tag, "DEBUG")) {
        va_end(args);
        return;
    }

    char bs[4][1024];

    for (usize i = 0; i < 4; i++) {
        bs[i][0] = '\0';
    }

    if (filename) {
        snprintf(bs[0], sizeof(bs[0]), " (%s:%llu)", filename, line);
    }

    if (fmt) {
        usize n = snprintf(bs[1], sizeof(bs[1]), "%s", ": ");
        vsnprintf(&bs[1][n], sizeof(bs[1]) - n, fmt, args);
    }

    if (token) {
        snprintf(
            bs[2], sizeof(bs[2]),
            " [%s:%lu]",
            token->input->name, token->line_no
        );

        char buf[1024];
        assert(token_line(token, buf, sizeof(buf)));
        usize n = snprintf(bs[3], sizeof(bs[3]), "\n%s\n", buf);

        for (
            usize i = 0;
            i < token->line_index && n < (sizeof(bs[3]) - 2);
            i++
        ) {
            bs[3][n++] = ' ';
        }

        bs[3][n] = '^';
        bs[3][n + 1] = '\0';
    }

    fprintf(
        stderr,
        "%s%s" ANSI_RESET "%s%s%s\n",
        tag, bs[0], bs[1], bs[2], bs[3]
    );
    va_end(args);
}

// allocates a buffer of at least n bytes which is cleaned up at exit time
void *asmalloc(usize n) {
    usize align = region && (((uptr) region->current) % 16 != 0) ?
        (16 - (((uptr) region->current) % 16)) :
        0;

    if (region == NULL || region->current + align + n >= region->end) {
        usize size = MAX(REGION_MIN_SIZE, 1 << (64 - __builtin_clz(n - 1)));
        void *allocated = malloc(size);
        assert(allocated);

       *((struct Region *) allocated) = (struct Region) {
            .current = allocated + sizeof(struct Region),
            .end = allocated + size,
            .next = region
        };

       region = allocated;
       align = 0;
       asmdbg(
            NULL, NULL,
            "Allocated new region of size 0x%08x at %p\n",
            size, region
        );
    }

    region->current += align;

    void *result = region->current;
    region->current += n;
    return result;
}

void *asmrealloc(void *ptr, usize n) {
    void *result = asmalloc(n);
    if (ptr) {
        memcpy(result, ptr, n);
    }
    return result;
}

int push_input_buffer(struct Context *ctx, const char *name, const char *buf) {
    if (ctx->input) {
        ctx->input->current = ctx->current;
    }

    struct Input *parent = ctx->input;

    ctx->input = asmalloc(sizeof(struct Input));
    *ctx->input = (struct Input) {
        .data = buf,
        .current = buf,
        .line = buf,
        .line_no = 1,
        .parent = parent
    };
    ctx->current = ctx->input->data;

    assert(strlen(name) < sizeof(ctx->input->name));
    strncpy(ctx->input->name, name, sizeof(ctx->input->name));

    asmdbg(ctx, NULL, "Pushed input %s of length %d", name, strlen(buf));
    return 0;
}

void pop_input(struct Context *ctx) {
    assert(ctx->input);
    ctx->input = ctx->input->parent;
    ctx->current = ctx->input ? ctx->input->current : NULL;
    asmdbg(
        ctx, NULL,
        "Popped input, now in %s",
        ctx->input ? ctx->input->name : "(NO INPUT)"
    );
}

int push_input(struct Context *ctx, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        return -1;
    }

    fseek(f, 0, SEEK_END);
    usize len = ftell(f);
    rewind(f);

    void *buf = asmalloc(len + 2);
    if (buf == NULL ||
        fread(buf, len, 1, f) != 1) {
        return -1;
    }
    fclose(f);
    ((char *) buf)[len + 0] = '\n';
    ((char *) buf)[len + 1] = '\0';

    return push_input_buffer(ctx, filename, buf);
}

// returns current location (address) in output
static inline u16 location(struct Context *ctx) {
    return (ctx->org == ORG_UNSET ? 0 : ctx->org) + ctx->out.size;
}

// emits types of arguments starting at Token tk to *types
// returns the number of argument types emitted.
// errors on un-typable things
static usize type_args(
        struct Context *ctx,
        struct Token *tk,
        enum Arg *args,
        struct Token **arg_tokens,
        usize args_capacity
    ) {
    char symbol[256];
    usize n = 0;
    isize i = 0, num_address = -1;

    while (tk->kind != TK_EOL) {
        assert(n < args_capacity);

        switch (tk->kind) {
            case TK_LBRACKET:
                num_address = 0;
                break;
            case TK_RBRACKET:
                asmchk(num_address > 0, ctx, tk, "Empty address");
                asmchk(
                    args[n - 1] & A_IMM, ctx, tk,
                    "Illegal address (not imm)"
                );
                args[n - 1] |= A_ADDR;
                num_address = -1;
                break;
            case TK_NUMBER:
                asmchk(tk->value <= 0xFFFF, ctx, tk, "Value out of bounds");
                arg_tokens[n] = tk;
                args[n++] = tk->value <= 0xFF ? A_IMM8 : A_IMM16;

                if (num_address != -1) {
                    asmchk(
                        num_address == 0, ctx, tk,
                        "Illegal address (>1 elements)"
                    );
                    num_address++;
                }
                break;
            case TK_DOT:
                asmchk(tk->next->kind == TK_SYMBOL, ctx, tk, "Expected label");
                tk = tk->next;
            case TK_SYMBOL:
                arg_tokens[n] = tk;

                assert(token_data(tk, symbol, sizeof(symbol)));
                if (num_address != -1) {
                    asmdbg(
                        ctx, NULL,
                        "Assuming type of %s is address", symbol
                    );
                    num_address++;
                    args[n++] = A_ADDR | A_IMM16;
                    break;
                }

                asmchk(
                    (i = strcasetab(R_NAMES, R_COUNT, symbol)) != -1,
                    ctx, tk,
                    "Unknown register %s", symbol
                );

                args[n++] = A_REG;
                break;
            default:
                asmchk(false, ctx, tk, "Unexpected token");
        }

        tk = tk->next;
    }

    return n;
}

// checks if arguments starting are legal for Op op
// returns NULL if arguments are legal, otherwise returns pointer to erroneous
// argument. returns pointer to last argument if there are too many arguments.
static enum Arg *check_args(
        struct Context *ctx,
        struct Op *op,
        enum Arg *args,
        usize num_args
    ) {

    enum Arg *arg = args, *op_arg = op->args;
    while (arg != &args[num_args]) {
        // too many arguments
        if (op_arg == &op->args[op->num_args]) {
            return &args[num_args - 1];
        }

        bool fail = false;
        fail |= (*op_arg & A_ADDR) && !(*arg & A_IMM);
        fail |= (*op_arg & A_REG) && !(*op_arg & A_IMM) && !(*arg & A_REG);
        fail |= (*op_arg & A_IMM) && !(*op_arg & A_REG) && !(*arg & A_IMM);
        fail |= (*op_arg & A_IMM8) && !(*op_arg & A_IMM16) && (*arg & A_IMM16);

        // TODO: is this check necessary?
        // fail |= !(*op_arg & A_ADDR) && (*arg & A_ADDR);

        if (fail) {
            if (*op_arg & A_HL) {
                // HL indicates argument is optional, retry with next argument
                op_arg++;
                continue;
            } else {
                return arg;
            }
        }

        arg++;
        op_arg++;
    }

    // not enough arguments
    if (op_arg != &op->args[op->num_args] &&
            !(*op_arg & A_HL)) {
        return &args[num_args - 1];
    }

    return NULL;
}

// returns parent label of label (previous non-sub) or NULL if there is no
// such label. returns NULL for NULL label, l for non-sub labels.
static struct Label *label_parent(struct Label *l) {
    while (l != NULL && l->sub) {
        l = l->next;
    }
    return l;
}

// resolves the specified label token to an address (*result) starting from
// the specified label as the last defined label (relevant for determining
// valid sub labels, etc.). returns non-zero on error.
static int resolve_label(
        struct Context *ctx,
        struct Token *label,
        struct Label *from,
        u16 *result
    ) {
    char symbol[256];
    struct Label *l = ctx->labels;

    from = from ? from : ctx->labels;

    assert(token_data(label, symbol, sizeof(symbol)));

    while (l != NULL) {
        if (!strcasecmp(symbol, l->name) &&
                (!l->sub || label_parent(l) == from)) {
            asmdbg(
                ctx, NULL,
                "Resolved label %s%s to %" PRIu16,
                (label->flags & (1 << TF_SUB)) ? "." : "", symbol, l->value
            );
            *result = l->value;
            return 0;
        }

        l = l->next;
    }

    asmdbg(
        ctx, label,
        "Could not resolve label %s%s",
        (label->flags & (1 << TF_SUB)) ? "." : "",  symbol
    );
    *result = 0;
    return -1;
}

// returns true if token is a symbol and register
static bool is_register(struct Token *token) {
    char symbol[256];

    if (token->kind != TK_SYMBOL) {
        return false;
    }

    assert(token_data(token, symbol, sizeof(symbol)));
    return strcasetab(R_NAMES, R_COUNT, symbol) != (usize) -1;
}

// retrieves register enum of the specified symbol token, assumes
// is_register(token) is true
static enum Register token_register(struct Token *token) {
    isize i;
    char symbol[256];

    assert(token->kind == TK_SYMBOL);
    assert(token_data(token, symbol, sizeof(symbol)));
    assert((i = strcasetab(R_NAMES, R_COUNT, symbol)) != -1);
    return (enum Register) i;
}

void eval_labels(
        struct Context *ctx,
        struct Token *start,
        struct Token *end
    ) {
    // replace any labels (and $s) with integer values
    struct Token *tk = start;
    while (tk != end->next) {
        if (tk->kind == TK_DOLLAR) {
            tk->kind = TK_NUMBER;
            tk->value = location(ctx);
            tk = tk->next;
            continue;
        } else if (tk->kind != TK_SYMBOL) {
            tk = tk->next;
            continue;
        } else if (tk->prev && tk->prev->kind == TK_AT) {
            tk = tk->next;
            continue;
        }

        // skip TK_DOT, mark label as sub
        if (tk->prev->kind == TK_DOT) {
            tk->prev->prev->next = tk;
            tk->flags |= (1 << TF_SUB);
        }

        // attempt to resolve label - could also be a register
        u16 value;
        if (!is_register(tk) && !resolve_label(ctx, tk, NULL, &value)) {
            tk->value = value;
            tk->kind = TK_NUMBER;
        }

        tk = tk->next;
    }
}

// adds the specified symbol as a patch, blocking out two bytes for the address
// in the output buffer
static void add_patch(struct Context *ctx, struct Token *symbol) {
    struct Label *parent = label_parent(ctx->labels);
    struct Patch *p = asmalloc(sizeof(struct Patch));

    p->label = parent;
    p->symbol = symbol;
    p->location = location(ctx);
    p->next = ctx->patches;
    ctx->patches = p;
    BUFADD(ctx->out, 0xF0);
    BUFADD(ctx->out, 0xF0);

    asmdbg(
        ctx, symbol,
        "Added patch (sub: %s, parent: %s) with value %" PRIu16,
        (symbol->flags & (1 << TF_SUB)) ? "yes" : "no",
        parent ? parent->name : "(null)",
        p->location
    );
}

// emits the specified token/argument as an address
static void emit_addr(struct Context *ctx, enum Arg arg, struct Token *token) {
    if (token->kind == TK_SYMBOL) {
        add_patch(ctx, token);
    } else {
        asmchk(token->kind == TK_NUMBER, ctx, token, "Expected number");
        asmchk(
            arg & A_IMM, ctx, token,
            "Expected immediate address"
        );
        BUFADD(ctx->out, token->value & 0xFF);
        BUFADD(ctx->out, (token->value >> 8) & 0xFF);
    }
}

// emits op starting at token start with the specified arguments/tokens to the
// output buffer
static void emit(
        struct Context *ctx,
        struct Op *op,
        struct Token *start,
        enum Arg *args,
        struct Token **arg_tokens,
        usize num_args
    ) {
    u8 i0 = op->instruction << 4, i1 = 0;

    enum Arg *error_arg;
    asmchk(
        !(error_arg = check_args(ctx, op, args, num_args)),
        ctx, arg_tokens[(usize) (error_arg - args)],
        "Illegal argument or not enough arguments"
    );

    switch (op->instruction) {
        case I_LW:
            // reg, [HL/imm16]
            i0 = BIT_SET(i0, 3, num_args != 2);
            i0 |= token_register(arg_tokens[0]) & 0x7;
            BUFADD(ctx->out, i0);

            if (num_args == 2) {
                emit_addr(ctx, args[1], arg_tokens[1]);
            }
            break;
        case I_SW:
            // [HL/imm16], reg
            i0 = BIT_SET(i0, 3, num_args != 2);
            i0 |= token_register(arg_tokens[num_args == 1 ? 0 : 1]);
            BUFADD(ctx->out, i0);

            if (num_args == 2) {
                emit_addr(ctx, args[0], arg_tokens[0]);
            }
            break;
        case I_PUSH:
        case I_JNZ:
            i0 = BIT_SET(i0, 3, args[0] & A_REG);
            i0 |= (args[0] & A_REG) ? token_register(arg_tokens[0]) : 0;
            BUFADD(ctx->out, i0);

            if (!(args[0] & A_REG)) {
                assert(arg_tokens[0]->kind == TK_NUMBER);
                BUFADD(ctx->out, arg_tokens[0]->value);
            }
            break;
        case I_POP:
            i0 |= (1 << 3);
            i0 |= token_register(arg_tokens[0]);
            BUFADD(ctx->out, i0);
            break;
        case I_LDA:
            // [imm16]
            BUFADD(ctx->out, i0);
            emit_addr(ctx, args[0], arg_tokens[0]);
            break;
        case I_OUTB:
            // imm8/reg, reg
            i0 = BIT_SET(i0, 3, args[0] & A_REG);
            i0 |= token_register(arg_tokens[1]);

            if (args[0] & A_REG) {
                i1 = token_register(arg_tokens[0]);
            } else {
                assert(args[0] & A_IMM8);
                assert(arg_tokens[0]->kind == TK_NUMBER);
                i1 = arg_tokens[0]->value;
            }

            BUFADD(ctx->out, i0);
            BUFADD(ctx->out, i1);
            break;
        default:
            // reg, imm8/reg
            i0 = BIT_SET(i0, 3, args[1] & A_REG);
            i0 |= token_register(arg_tokens[0]) & 0x7;

            if (args[1] & A_REG) {
                i1 = token_register(arg_tokens[1]);
            } else {
                assert(args[1] & A_IMM8);
                assert(arg_tokens[1]->kind == TK_NUMBER);
                i1 = arg_tokens[1]->value;
            }

            BUFADD(ctx->out, i0);
            BUFADD(ctx->out, i1);
            break;
    }
}

// lexer line end callback. used to trigger lex directives.
static void lex_line(struct Context *ctx, struct Token *first) {
    if (first->kind == TK_AT) {
        asmchk(first->next->kind == TK_SYMBOL, ctx, first, "Expected directive");

        char directive_name[32];
        assert(token_data(first->next, directive_name, sizeof(directive_name)));
        directive(ctx, first, true);
    }
}

// parses and emits the specified token stream
static struct Token *parse(struct Context *ctx, struct Token *first) {
    if (first->kind == TK_EOL) {
        return first->next;
    }

    struct Token *last, *eol = first;
    while (eol->kind != TK_EOL) {
        eol = eol->next;
    }
    last = eol->prev;

    if (first->kind == TK_AT) {
        asmchk(
            first->next->kind == TK_SYMBOL,
            ctx, first, "Expected directive"
        );
        directive(ctx, first, false);
        return eol->next;
    }

    // label
    if (last->kind == TK_COLON) {
        if (ctx->macro) {
            // macro ends on first empty line (double EOL)
            struct Token *macro_end = last->next;

            while (macro_end->next &&
                    !(macro_end->kind == TK_EOL &&
                        macro_end->next->kind == TK_EOL)) {
                macro_end = macro_end->next;
            }

            parse_macro(ctx, first, macro_end);
            ctx->macro = false;
            return macro_end->next;
        } else if (!ctx->microcode) {
            struct Label *label = asmalloc(sizeof(struct Label));

            bool sub = false;
            struct Token *name = first;

            if (name->kind == TK_DOT) {
                sub = true;
                name = name->next;
            }

            asmchk(name->kind == TK_SYMBOL, ctx, name, "Malformed label");

            *label = (struct Label) {
                .name = token_data(name, NULL, 0),
                .sub = sub,
                .value = location(ctx),
                .prev = NULL,
                .next = ctx->labels
            };

            if (ctx->labels) {
                ctx->labels->prev = label;
            }

            ctx->labels = label;
            return eol->next;
        }
    }

    asmchk(!ctx->macro, ctx, first, "Expected macro");

    if (ctx->microcode) {
        return microcode_parse(ctx, first);
    }

    // evaluate labels
    eval_labels(ctx, first->next, eol->prev);

    // evaluate arguments into final values
    struct Token *tk;
    asmchk(
        !(tk = eval_between(ctx, first->next, eol->prev)),
        ctx, tk,
        "Error parsing expression"
    );

    asmchk(first->kind == TK_SYMBOL, ctx, first, "Malformed syntax");

    // must be an operation
    char op_name[256];
    assert(token_data(first, op_name, sizeof(op_name)));

    // search for the right operation which also type checks
    usize num_matching = 0;
    struct Op *op = &ctx->ops.buffer[0], *matching_ops[16];

    enum Arg args[16], *error_arg;
    struct Token *arg_tokens[16];
    usize num_args = type_args(ctx, first->next, args, arg_tokens, ARRLEN(args));

    // find a matching operation
    while (op != &ctx->ops.buffer[ctx->ops.size]) {
        if (strcasecmp(op_name, op->name)) {
            op++;
            continue;
        }

        matching_ops[num_matching++] = op;

        // check that args can match
        if (!(error_arg = check_args(ctx, op, args, num_args))) {
            break;
        }

        op++;
    }

    if (op == &ctx->ops.buffer[ctx->ops.size]) {
        asmchk(num_matching > 0, ctx, first, "Unknown operation");
        asmchk(
            num_matching != 1, ctx,
            error_arg ? arg_tokens[(usize) (error_arg - args)] : first,
            "Argument type mismatch"
        );
        asmchk(
            false, ctx, first,
            "TODO: good error message for 'could not find op'"
        );
    }

    if (op->macro) {
        return expand_macro(
            ctx, op, first, eol->prev,
            args, arg_tokens, num_args
        );
    }

    emit(ctx, op, first, args, arg_tokens, num_args);

    return eol->next;
}

static void assemble(struct Context *ctx) {
    struct Token *tokens = lex(ctx, lex_line),
                 *t = tokens;

    while (t) {
        if (t->kind != TK_EOL) {
            asmdbg(ctx, t, "parsing");
        }

        t = parse(ctx, t);
    }

    // emit microcode after parsing, no labels to resolve
    if (ctx->microcode) {
        microcode_emit(ctx);
        return;
    }

    // resolve patches
    struct Patch *p = ctx->patches;
    while (p != NULL) {
        u16 value;
        asmchk(
            !resolve_label(ctx, p->symbol, p->label, &value),
            ctx, p->symbol,
            "Could not patch label %s%s",
            (p->symbol->flags & (1 << TF_SUB)) ? "." : "",
            token_data(p->symbol, NULL, 0)
        );
        ctx->out.buffer[p->location + 0] = (value >> 0) & 0xFF;
        ctx->out.buffer[p->location + 1] = (value >> 8) & 0xFF;
        p = p->next;
    }
}

static void usage() {
    printf(
        "usage: asm [-h] [--help] [-v] [--verbose] [-n]"
        "[--no-builtin-macros] [-o file] file\n"
    );
}

int main(int argc, const char *argv[]) {
    const char *infile = NULL, *outfile = "out.bin";
    bool builtin_macros = true;
    struct Context ctx;
    memset(&ctx, 0, sizeof(struct Context));

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") ||
                !strcmp(argv[i], "--help")) {
            usage();
            exit(0);
        } else if (!strcmp(argv[i], "-o")) {
            asmchk(i + 1 < argc, NULL, NULL, "Expected output filename");
            outfile = argv[++i];
        } else if (!strcmp(argv[i], "-v") ||
                !strcmp(argv[i], "--verbose")) {
            ctx.verbose = true;
        } else if (!strcmp(argv[i], "-n") ||
                !strcmp(argv[i], "--no-builtin-macros")) {
            builtin_macros = false;
        } else {
            asmchk(
                infile == NULL, NULL, NULL,
                "Multiple input files specified"
            );
            infile = argv[i];
        }
    }

    asmchk(infile != NULL, NULL, NULL, "No input file specified");

    ctx.org = ORG_UNSET;
    ctx.ops.capacity_min = 16;
    ctx.out.capacity_min = 0x8000;

    for (usize i = 0; i < ARRLEN(INSTRUCTIONS); i++) {
        BUFADDP(ctx.ops, &INSTRUCTIONS[i]);
    }

    // push one input, automatically popped
    asmchk(!push_input(&ctx, infile), NULL, NULL, "Error reading input file");

    // push macros. add null terminator since xxd doesn't null terminate >:(
    if (builtin_macros) {
        char *builtin_macros_text = asmalloc(BUILTIN_MACROS_LEN + 1);
        memcpy(builtin_macros_text, BUILTIN_MACROS_TEXT, BUILTIN_MACROS_LEN);
        builtin_macros_text[BUILTIN_MACROS_LEN] = '\0';
        assert(!push_input_buffer(&ctx, "(BUILT-IN MACROS)", builtin_macros_text));
    }

    assemble(&ctx);

    FILE *out = fopen(outfile, "w+");
    assert(out);
    assert(fwrite(ctx.out.buffer, ctx.out.size, 1, out) == 1);
    assert(!fclose(out));

    // free allocated memory regions
    while (region) {
        struct Region *old = region;
        region = region->next;
        free(old);
    }
}
