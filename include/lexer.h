#pragma once

#include "defs.h"
#include "str.h"
#include "utf8.h"
#include "util.h"

#include <stdio.h>

#define C_EOF UINT32_MAX

extern struct string const token_names[];

enum lex_token_type {
    TOKEN_NONE,
    TOKEN_NAME,
    TOKEN_ASSIGN,
    TOKEN_DOT,
    TOKEN_DOT_DOT_DOT,
    TOKEN_COMMA,
    TOKEN_CHAR,
    TOKEN_STRING,
    TOKEN_OPEN_BRACE,
    TOKEN_CLOSE_BRACE,
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_GREATER_THAN,
    TOKEN_SEMICOLON,
    TOKEN_INTEGER,
    TOKEN_EOF
};

struct token {
    struct location location;
    enum lex_token_type type;
    union {
        struct string string;
        struct string name;
        char character;
        u32 integer;
    };
};

struct lexer {
    struct allocator* allocator;
    FILE* in;
    char* buffer;
    usize buffer_size;
    usize buffer_length;
    struct codepoint c[2];
    struct token un;
    struct location location;
};

struct allocator;

struct lexer lexer_new(
    struct allocator allocator[static const 1], FILE f[static const 1],
    i32 const file_id
);
void lexer_finish(struct lexer lexer[static const 1]);
enum lex_token_type lex(
    struct lexer lexer[static const 1], struct token out[static const 1]
);
void unlex(
    struct lexer lexer[static const 1], struct token out[static const 1]
);
void token_finish(struct token token[static const 1]);
