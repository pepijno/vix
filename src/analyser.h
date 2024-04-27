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
    Scope root_scope;
    ErrorMessage** errors;
    ErrorColor error_color;
    Allocator* allocator;
} Analyzer;

void analyse(
    Analyzer code_gen[static 1],
    AstNode root[static 1]
);
