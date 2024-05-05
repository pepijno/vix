#pragma once

#include "defs.h"
#include "utf8.h"
#include "util.h"

#include <stdio.h>

#define C_EOF UINT32_MAX

extern char const* token_names[];

enum lex_token_type_e {
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

struct token_t {
    struct location_t location;
    enum lex_token_type_e type;
    union {
        struct {
            char* value;
            i32 length;
        } string;
        char* name;
        char character;
        u32 integer;
    };
};

struct lexer_t {
    FILE* in;
    char* buffer;
    i32 buffer_size;
    i32 buffer_length;
    struct codepoint_t c[2];
    struct token_t un;
    struct location_t location;
};

struct lexer_t lexer_new(FILE* f, i32 file_id);
void lexer_finish(struct lexer_t lexer[static 1]);
enum lex_token_type_e lex(
    struct lexer_t lexer[static 1], struct token_t out[static 1]
);
void unlex(struct lexer_t lexer[static 1], struct token_t out[static 1]);
void token_finish(struct token_t token[static 1]);
