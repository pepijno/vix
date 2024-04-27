#pragma once

#include "allocator.h"
#include "string.h"

typedef enum {
    TOKEN_IDENTIFIER,
    TOKEN_ASSIGN,
    TOKEN_DOT,
    TOKEN_DOT_DOT,
    TOKEN_DOT_DOT_DOT,
    TOKEN_COMMA,
    TOKEN_CHAR_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_OPEN_BRACE,
    TOKEN_CLOSE_BRACE,
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_GREATER_THAN,
    TOKEN_SEMICOLON,
    TOKEN_INT,
    TOKEN_EOF
} TokenType;

typedef struct {
    Str identifier;
} TokenIdentifier;

typedef struct {
    Str string;
} TokenString;

typedef struct {
    u8 c;
} TokenChar;

typedef struct {
    Str integer;
} TokenInteger;

typedef struct {
    TokenType type;
    i64 start_position;
    i64 end_position;
    i64 start_line;
    i64 start_column;
    union {
        TokenString string_literal;
        TokenIdentifier identifier;
        TokenChar char_literal;
        TokenInteger integer;
    };
} Token;

typedef Token** TokenPtrArray;
typedef struct {
    TokenPtrArray* tokens;
    i64* line_offsets;

    Str error;
    i64 error_line;
    i64 error_column;
} Tokenized;

void tokenize(
    Allocator allocator[static 1], Str source,
    TokenPtrArray tokens[static 1], Tokenized out[static 1]
);
Str token_name(TokenType type);
void print_tokens(Str source, TokenPtrArray tokens);
