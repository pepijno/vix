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
} token_type_e;

typedef struct {
    str_t identifier;
} token_identifier_t;

typedef struct {
    str_t string;
} token_string_t;

typedef struct {
    char c;
} token_char_t;

typedef struct {
    str_t integer;
} token_integer_t;

typedef struct {
    token_type_e type;
    size_t start_position;
    size_t end_position;
    size_t start_line;
    size_t start_column;
    union {
        token_string_t string_literal;
        token_identifier_t identifier;
        token_char_t char_literal;
        token_integer_t integer;
    } data;
} token_t;

typedef token_t** token_ptr_array_t;
typedef struct {
    token_ptr_array_t* tokens;
    size_t* line_offsets;

    str_t error;
    size_t error_line;
    size_t error_column;
} tokenized_t;

void tokenize(
    allocator_t allocator[static 1], str_t source,
    token_ptr_array_t tokens[static 1], tokenized_t out[static 1]
);
char * token_name(token_type_e type);
void print_tokens(str_t source, token_ptr_array_t tokens);
