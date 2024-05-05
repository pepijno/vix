#pragma once

// #include "string.h"
// #include "parser.h"
// #include "analyser.h"
//
// typedef struct {
//     Scope root_scope;
//     Allocator* allocator;
// } CodeGen;
//
// void generate(
//     CodeGen code_gen[static 1],
//     StrBuffer buffer[static 1],
//     AstNode root[static 1]
// );

#include "parser.h"

void
generate(struct ast_object_t root[static 1]);
