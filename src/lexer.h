#pragma once

#include "allocator.h"
#include "string.h"

enum token_type_t {
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
};

struct token_identifier_t {
    struct str_t identifier;
};

struct token_string_t {
    struct str_t string;
};

struct token_char_t {
    char c;
};

struct token_integer_t {
    struct str_t integer;
};

struct token_t {
    enum token_type_t type;
    size_t start_position;
    size_t end_position;
    size_t start_line;
    size_t start_column;
    union {
        struct token_string_t string_literal;
        struct token_identifier_t identifier;
        struct token_char_t char_literal;
        struct token_integer_t integer;
    } data;
};

typedef struct token_t** token_ptr_array_t;
struct tokenized_t {
    token_ptr_array_t* tokens;
    size_t* line_offsets;

    struct str_t error;
    size_t error_line;
    size_t error_column;
};

void tokenize(
    struct allocator_t allocator[static const 1], struct str_t const source,
    token_ptr_array_t tokens[static const 1],
    struct tokenized_t out[static const 1]
);
char const* token_name(enum token_type_t const type);
void print_tokens(struct str_t source, token_ptr_array_t tokens);
