#ifndef LEX_H
#define LEX_H

#include "../common/util.h"

// forward declarations from asm.h
struct Input;
struct Context;

#define TK_COUNT (TK_GREATER + 1)
enum TokenKind {
    TK_NOP = 0,
    TK_EOL,
    TK_SYMBOL,
    TK_NUMBER,
    TK_STRING,
    TK_LPAREN,
    TK_RPAREN,
    TK_AT,
    TK_LBRACKET,
    TK_RBRACKET,
    TK_COLON,
    TK_PERCENT,
    TK_PLUS,
    TK_MINUS,
    TK_AMPERSAND,
    TK_PIPE,
    TK_NOT,
    TK_DOLLAR,
    TK_STAR,
    TK_CARET,
    TK_SLASH,
    TK_DOT,
    TK_LESS,
    TK_GREATER
};

struct Token {
    enum TokenKind kind;

    // neighboring tokens in stream
    struct Token *prev, *next;

    // origin info
    const struct Input *input;
    const char *line;
    usize line_no, line_index;

    // for TK_SYMBOL, TK_STRING, TK_NUMBER
    const char *data;
    usize len;

    // for TK_NUMBER
    i64 value;

    // for arbitrary information on all tokens
    u64 flags;
};

const char TOKEN_VALUES[TK_COUNT];

char *token_data(const struct Token *token, char *buf, usize n);
char *token_line(const struct Token *token, char *buf, usize n);
char *token_between(
    const struct Token *start,
    const struct Token *end,
    char *buf,
    usize n
);
struct Token *lex(
    struct Context *ctx,
    void (*linecb)(struct Context*, struct Token*)
);

#endif
