#include "lex.h"
#include "asm.h"

const char TOKEN_VALUES[TK_COUNT] = {
    [TK_EOL]        = 0,
    [TK_SYMBOL]     = 0,
    [TK_NUMBER]     = 0,
    [TK_STRING]     = 0,
    [TK_LPAREN]     = '(',
    [TK_RPAREN]     = ')',
    [TK_AT]         = '@',
    [TK_LBRACKET]   = '[',
    [TK_RBRACKET]   = ']',
    [TK_COLON]      = ':',
    [TK_PERCENT]    = '%',
    [TK_PLUS]       = '+',
    [TK_MINUS]      = '-',
    [TK_AMPERSAND]  = '&',
    [TK_PIPE]       = '|',
    [TK_NOT]        = '~',
    [TK_DOLLAR]     = '$',
    [TK_STAR]       = '*',
    [TK_CARET]      = '^',
    [TK_SLASH]      = '/',
    [TK_DOT]        = '.',
    [TK_LESS]       = '<',
    [TK_GREATER]    = '>',
};

// retrieves data from token into buf of size n. if buf is NULL, a buffer of
// adequate size is asmalloc'd and returned. returns NULL on failure.
char *token_data(const struct Token *token, char *buf, size_t n) {
    assert(token->data);

    if (buf == NULL) {
        buf = asmalloc(token->len + 1);
    } else if (n < token->len + 1) {
        return NULL;
    }

    memcpy(buf, token->data, token->len);
    buf[token->len] = '\0';
    return buf;
}

// retrieves the entire line of a token into buf of size n. if buf is NULL, a
// buffer of adequate size is asmalloc'd and returned. returns NULL on failure.
char *token_line(const struct Token *token, char *buf, size_t n) {
    const char *end = strchr(token->line, '\n');
    const usize llen =
        end == NULL ?
            strlen(token->line) :
            ((usize) (end - token->line));

    if (buf == NULL) {
        buf = asmalloc(llen + 1);
    } else if (llen + 1 > n) {
        return NULL;
    }

    memcpy(buf, token->line, llen);
    buf[llen] = '\0';

    return buf;
}

// retrieves the string data between start and end (inclusive) into buf of size
// n. if buf is NULL, a buffer of adequate size is asmalloc'd and returned.
// returns NULL on failure.
char *token_between(
        const struct Token *start,
        const struct Token *end,
        char *buf,
        usize n
    ) {
    // count total number of tokens, inclusive
    const struct Token *t = start;
    usize num_tokens = 0;
    while (t != end->next) {
        num_tokens++;
        t = t->next;
    }

    // count characters in each token, including whitespace/ignored chars after
    char *p;
    usize i = 0, j = 0, len = 0, lens[num_tokens];
    t = start;
    while (t != end->next) {
        j = 0;
        while ((p = strchr(" ,\t", t->data[t->len + j])) && *p != '\0') {
            j++;
        }

        lens[i] = t->len + j;
        i++;
        t = t->next;
    }

    if (buf == NULL) {
        buf = asmalloc(len + 1);
    } else if (len + 1 > n) {
        return NULL;
    }

    // copy into buffer
    i = 0;
    len = 0;
    t = start;
    while (t != end->next) {
        memcpy(&buf[len], t->data, lens[i]);
        len += lens[i];
        i++;
        t = t->next;
    }

    buf[len] = '\0';
    return buf;
}

// does some preliminary parsing on symbol tokens to identify them
static void kind_symbol(struct Token *token) {
    char data[256];
    assert(token->kind == TK_SYMBOL);
    assert(token_data(token, data, sizeof(data)));

    i64 l;
    if (!strtoi64(data, 0, &l)) {
        token->kind = TK_NUMBER;
        token->value = l;
    }
}

// expands a potentially @define'd symbol
static bool expand_define(struct Context *ctx, struct Token *token) {
    char data[256];
    assert(token->kind == TK_SYMBOL);
    assert(token_data(token, data, sizeof(data)));

    struct Define *d = ctx->defines;
    while (d != NULL && strcasecmp(data, d->name)) {
        d = d->next;
    }

    if (d == NULL) {
        return false;
    }

    asmdbg(
        ctx, token,
        "Expanding %s to %s in %s:%lu\n",
        data, d->value, token->input->name, token->line_no
    );
    assert(!push_input_buffer(ctx, "(define)", d->value));
    return true;
}

// pushes a token to the token stream, returning the latest in the stream
static struct Token *lex_push(
        struct Context *ctx,
        struct Token *current,
        struct Token token
    ) {
    current->next = asmalloc(sizeof(struct Token));
    token.prev = current;
    token.next = NULL;
    token.input = ctx->input;
    token.line = ctx->input->line;
    token.line_no = ctx->input->line_no;
    token.flags = 0;
    *current->next = token;
    return current->next;
}

// pushes a data token to the token stream, returning the latest in the stream
static struct Token *lex_data(
        struct Context *ctx,
        struct Token *current,
        enum TokenKind kind,
        const char *data,
        usize len
    ) {
    return lex_push(ctx, current, (struct Token) {
        .kind = kind,
        .data = data,
        .len = len,
        .line_index = data - ctx->input->line
    });
}

// pushes a value token to the token stream, returning the latest in the stream
static struct Token *lex_value(
        struct Context *ctx,
        struct Token *current,
        enum TokenKind value
    ) {
    return lex_push(ctx, current, (struct Token) {
        .kind = value,
        .data = ctx->current,
        .len = 1,
        .line_index = ctx->current - ctx->input->line
    });
}

// lexes a potentially escaped charater, returning its escaped value if
// applicable. otherwise just returns the character.
static char lex_potentially_escaped(struct Context *ctx, struct Token *token) {
    bool escaped = *ctx->current == '\\';

    asmchk(
        !escaped || isprint(*(ctx->current + 1)), ctx, token,
        "Expected character to escape"
    );

    char c = escaped ? *(ctx->current + 1) : *ctx->current;

    if (escaped) {
        c = escchar(c);
    }

    ctx->current += escaped ? 2 : 1;
    return c;
}

// tokenizes whatever input is currently available in ctx, calling back into
// linecb() with the first token of every line read.
// returns a pointer to the start of the token stream, NULL if there are no
// tokens in the stream.
struct Token *lex(
        struct Context *ctx,
        void (*linecb)(struct Context*, struct Token*)
    ) {
    // first/current token and start of current line
    struct Token *first = asmalloc(sizeof(struct Token)),
                 *token = first;

    // first token is NOP, should be skipped on return
    *first = (struct Token) { .kind = TK_NOP };

    // pushes start to the token stream if it exists
    // TODO: this is kind of a garbage macro (complicated, modifies control flow,
    // etc.) but I can't think of a better solution at the moment.
#define lex_symbol()                                                    \
    if (start != NULL) {                                                \
        token = lex_data(                                               \
            ctx,                                                        \
            token,                                                      \
            TK_SYMBOL,                                                  \
            start,                                                      \
            ctx->current - start                                        \
        );                                                              \
        start = NULL;                                                   \
        if (expand_define(ctx, token)) {                                \
            token = token->prev;                                        \
            goto next_it;                                               \
        } else {                                                        \
            kind_symbol(token);                                         \
        }                                                               \
    }

    // start of current symbol being read
    const char *start = NULL;

    bool in_str = false, escaped = false;

    while (ctx->current != NULL) {
        // current character is escaped if prefixed with non-escaped backslash
        escaped =
            ctx->current != ctx->input->data &&
            ctx->current[-1] == '\\' &&
            (ctx->current == (ctx->input->data + 1) ||
             ctx->current[-2] != '\\');

        // read string literal if in_str
        if (in_str) {
            asmchk(*ctx->current != '\0', ctx, token, "Unfinished string");

            if (!escaped && *ctx->current == '\"') {
                in_str = false;
                token = lex_data(
                    ctx, token,
                    TK_STRING, start + 1, ctx->current - start - 1
                );
                start = NULL;
            }

            ctx->current++;
            continue;
        }

        switch (*ctx->current) {
            case '\0':
            case EOF:
                lex_symbol();
                pop_input(ctx);
                continue;
            case '\t':
            case ' ':
            case ',':
                lex_symbol();
                break;
            case ';':
                // EOL when comment (;) is encountered
                if (escaped) {
                    break;
                }
            case '\r':
            case '\n':
                lex_symbol();
                token = lex_value(ctx, token, TK_EOL);

                // seek to character after next newline
                ctx->current = strpbrk(ctx->current, "\n");
                ctx->current = ctx->current ?
                    (ctx->current + 1) :
                    (ctx->current + strlen(ctx->current) + 1);
                ctx->input->line = ctx->current;

                // trace backwards to find line start
                struct Token *line_start = token;
                while (line_start->prev->kind != TK_EOL &&
                        line_start->prev != first) {
                    line_start = line_start->prev;
                }

                if (linecb != NULL) {
                    linecb(ctx, line_start);
                }

                line_start = NULL;
                ctx->input->line_no++;
                continue;
            case '\"':
                if (!escaped) {
                    lex_symbol();
                    start = ctx->current;
                    in_str = true;
                    break;
                }
            case '\'':
                if (!escaped) {
                    lex_symbol();
                    const char *char_start = ctx->current;
                    ctx->current++;
                    asmchk(
                        isprint(*ctx->current), ctx, token,
                        "Expected continuation of character"
                    );
                    char v = lex_potentially_escaped(ctx, token);
                    asmchk(
                        *ctx->current == '\'', ctx, token,
                        "Expected end of character"
                    );
                    token = lex_push(ctx, token, (struct Token) {
                        .kind = TK_NUMBER,
                        .data = ctx->current,
                        .len = ctx->current - char_start + 1,
                        .line_index = char_start - ctx->input->line,
                        .value = v
                    });
                    break;
                }
            default:
                // try to read single character tokens
                if (!escaped) {
                    for (usize i = 0; i < TK_COUNT; i++) {
                        if (*ctx->current != TOKEN_VALUES[i]) {
                            continue;
                        }

                        lex_symbol();
                        token = lex_value(ctx, token, i);
                        start = NULL;
                        goto next_char;
                    }
                }

                // starting a new symbol
                if (start == NULL) {
                    start = ctx->current;
                }

                // check that this character is legal in a symbol
                asmchk(
                    isalpha(*ctx->current) ||
                        isdigit(*ctx->current) ||
                        *ctx->current == '_',
                    ctx,
                    token,
                    "Illegal character '%c' in symbol",
                    *ctx->current
                );
        }
next_char:
        ctx->current++;
next_it:
        continue;
    }

    // first->next to skip over initial NOP token
    return first->next;
}

