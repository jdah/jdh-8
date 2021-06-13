#include "asm.h"

struct Directive {
    const char *name;
    bool lex;
    void (*handler)(struct Context *, struct Token *);
};

static void dir_microcode(struct Context *ctx, struct Token *first) {
    asmchk(first->next->next->kind == TK_EOL, ctx, first, "Invalid directive");
    asmdbg(ctx, NULL, "Microcode mode enabled");
    ctx->microcode = true;
}

static void dir_include(struct Context *ctx, struct Token *first) {
    struct Token *path = first->next->next;

    char filename[256];
    asmchk(path->kind == TK_STRING, ctx, path, "Invalid path");
    assert(token_data(path, filename, sizeof(filename)));
    strstrip(filename);

    asmchk(
        !push_input(ctx, filename),
        ctx,
        path,
        "Cannot open file %s",
        filename
     );
}

static void dir_define(struct Context *ctx, struct Token *first) {
    struct Token *name = first->next->next;
    asmchk(name, ctx, name, "Malformed @define");

    char *def_name = token_data(name, NULL, 0),
         *def_value = NULL;

    assert(def_name);

    if (name->next->kind != TK_EOL) {
        struct Token *eol = name;
        while (eol->kind != TK_EOL) {
            eol = eol->next;
        }

        def_value = token_between(name->next, eol->prev, NULL, 0);
        assert(def_value);
        strstrip(def_value);
    }

    asmchk(
        strcasecmp(def_name, def_value),
        ctx, name,
        "%s defined as itself, this will not end well",
        def_name
    );

    // check for redefinition
    struct Define *d = ctx->defines;
    while (d != NULL) {
        if (!strcasecmp(def_name, d->name)) {
            asmwrn(ctx, name, "Redefinition of %s", def_name);
            break;
        }

        d = d->next;
    }

    struct Define *old = ctx->defines;
    ctx->defines = asmalloc(sizeof(struct Define));
    *ctx->defines = (struct Define) {
        .name = def_name,
        .value = def_value,
        .next = old
    };

    asmdbg(
        ctx, NULL,
        "Added define %s=%s",
        def_name, def_value
    );
}

static void dir_macro(struct Context *ctx, struct Token *first) {
    ctx->macro = true;
}

static void eval_expressions(struct Context *ctx, struct Token *first) {
    struct Token *eol = first;
    while (eol->kind != TK_EOL) {
        eol = eol->next;
    }

    eval_labels(ctx, first->next->next, eol->prev);

    struct Token *tk;
    asmchk(
        !(tk = eval_between(ctx, first->next->next, eol->prev)),
        ctx, tk,
        "Error parsing expression"
    );
}

static void dir_ds(struct Context *ctx, struct Token *first) {
    struct Token *str = first->next->next;
    asmchk(str->kind == TK_STRING, ctx, str, "Expected string");

    const char *data = token_data(str, NULL, 0);

    // TODO: do escaped character processing elsewhere
    for (usize i = 0; i < str->len; i++) {
        BUFADD(ctx->out, data[i] == '\\' ? escchar(data[++i]) : data[i]);
    }

    BUFADD(ctx->out, 0);
}

static void dir_db(struct Context *ctx, struct Token *first) {
    struct Token *t = first->next->next, *eol = t;

    while (eol->kind != TK_EOL) {
        eol = eol->next;
    }

    eval_expressions(ctx, first);
    eval_labels(ctx, t, eol->prev);

    while (t->kind != TK_EOL) {
        asmchk(t->kind == TK_NUMBER, ctx, t, "Expected byte");
        asmchk(
            t->value >= INT8_MIN && t->value <= UINT8_MAX,
            ctx, t,
            "Value out of bounds"
        );

        BUFADD(ctx->out, (u8) (t->value));
        t = t->next;
    }
}

static void dir_dd(struct Context *ctx, struct Token *first) {
    struct Token *t = first->next->next, *eol = t;

    while (eol->kind != TK_EOL) {
        eol = eol->next;
    }

    eval_expressions(ctx, first);
    eval_labels(ctx, t, eol->prev);

    while (t->kind != TK_EOL) {
        if (t->kind == TK_LBRACKET)  {
            asmchk(
                t->next->kind == TK_NUMBER &&
                t->next->next->kind == TK_RBRACKET,
                ctx, t,
                "Malformed address"
            );
            t = t->next;
            t->next = t->next->next;
            t->next->prev = t;
        }

        asmchk(t->kind == TK_NUMBER, ctx, t, "Expected double word");
        asmchk(
            t->value >= INT16_MIN && t->value <= UINT16_MAX,
            ctx, t,
            "Value out of bounds"
        );

        u16 value = t->value;
        BUFADD(ctx->out, (value >> 0) & 0xFF);
        BUFADD(ctx->out, (value >> 8) & 0xFF);
        t = t->next;
    }
}

static void dir_dn(struct Context *ctx, struct Token *first) {
    struct Token *n = first->next->next;
    asmchk(n->kind == TK_NUMBER, ctx, n, "Expected number")
    asmchk(n->next->kind == TK_EOL, ctx, n->next, "Expected line end");

    u16 v = n->value;
    while (v > 0) {
        BUFADD(ctx->out, 0);
        v--;
    }
}

static void dir_org(struct Context *ctx, struct Token *first) {
    asmchk(ctx->org == ORG_UNSET, ctx, first, "Multiple values for origin");

    struct Token *value = first->next->next;
    asmchk(
        value->kind == TK_NUMBER && value->next->kind == TK_EOL,
        ctx, value,
        "Illegal arguments to org directive"
    );

    i64 org = value->value;
    asmchk(
        org >= 0 && org <= 0xFFFF, ctx, value,
        "Origin must be in range 0x0000..0xFFFF"
    );
    ctx->org = org;
    asmdbg(ctx, NULL, "Org set to %" PRIu16, ctx->org);
}

static const struct Directive DIRECTIVES[] = {
    { "microcode", true, dir_microcode },
    { "include", true, dir_include },
    { "define",  true, dir_define },
    { "macro", false, dir_macro },
    { "ds", false, dir_ds },
    { "db", false, dir_db },
    { "dd", false, dir_dd },
    { "dn", false, dir_dn },
    { "org", false, dir_org },
};

void directive(
       struct Context *ctx,
       struct Token *first,
       bool lex
    ) {
    assert(first->kind == TK_AT);
    assert(first->next->kind == TK_SYMBOL);

    char symbol[256];
    assert(token_data(first->next, symbol, sizeof(symbol)));

    asmdbg(ctx, first, "Processing directive %s", symbol);

    for (usize i = 0; i < ARRLEN(DIRECTIVES); i++) {
        const struct Directive *d = &DIRECTIVES[i];

        if (!strcasecmp(d->name, symbol)) {
            if (lex == d->lex) {
                d->handler(ctx, first);
            }

            // skip processing and don't error if name matches but in wrong step
            return;
        }
    }

    // only fail on unrecognized non-lex directives
    asmchk(lex, ctx, first->next, "Unrecognized directive");
}
