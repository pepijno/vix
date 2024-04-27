#pragma once

#include "string.h"
#include "parser.h"
#include "analyser.h"

typedef struct {
    Str str;
    i64 id;
} StringData;

typedef struct {
    Scope root_scope;
    Allocator* allocator;
    StringData* data_strings;
} CodeGen;

void generate(
    CodeGen code_gen[static 1],
    StrBuffer buffer[static 1],
    AstNode root[static 1]
);
