#pragma once

#include "defs.h"

#define BUCKET_SIZE 4096

struct arena;
struct lexer;

enum parser_entry_type {
    PARSER_ENTRY_TYPE_OBJECT,
    PARSER_ENTRY_TYPE_PROPERTY,
};

struct parser_entry {
    enum parser_entry_type type;
    union {
        struct ast_object* object;
        struct ast_property* property;
    };
    struct parser_entry* next;
};

extern struct parser_entry* buckets[BUCKET_SIZE];
struct parser_entry* buckets_lookup(u32 const id);

struct ast_object* parse(struct arena* arena, struct lexer* lexer);
void print_object(struct ast_object* object, usize indent);
