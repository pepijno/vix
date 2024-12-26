#pragma once

#include "ast.h"
#include "defs.h"
#include "types.h"

void print_element(struct ast_element const element, usize const indent);
void print_type(
    struct type_context const context, struct type const* const type,
    usize const indent
);
void print_instruction(
    struct instruction const instruction, usize const indent
);
