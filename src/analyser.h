#pragma once

#include "allocator.h"
#include "parser.h"

typedef struct Scope Scope;
struct Scope {
    AstNode const* const source_node;
    Scope* parent;
    AstNode** properties;
};

typedef struct {
    Str str;
    i64 id;
} StringData;

typedef struct {
    Scope root_scope;
    ErrorMessage** errors;
    i64 error_line;
    i64 error_column;
    ErrorColor error_color;
    Allocator* allocator;
    StringData* data_strings;
} CodeGen;

void analyse(
    CodeGen code_gen[static 1],
    AstNode root[static 1]
);

void generate(
    CodeGen code_gen[static 1],
    StrBuffer buffer[static 1],
    AstNode root[static 1]
);
