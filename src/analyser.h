#pragma once

#include "allocator.h"
#include "parser.h"

struct scope_t {
    struct ast_node_t const* const source_node;
    struct scope_t* parent;
    struct ast_node_t const** properties;
};

struct code_gen_t {
    struct scope_t root_scope;
    struct error_message_t** errors;
    size_t error_line;
    size_t error_column;
    enum error_color_t error_color;
};

void analyse(
    struct allocator_t* allocator, struct ast_node_t const* const root
);
