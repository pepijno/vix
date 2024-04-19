#pragma once

#include "allocator.h"
#include "parser.h"

struct scope_t {
    struct ast_node_t const* const source_node;
    struct scope_t* parent;
    struct ast_node_t const** properties;
};

struct string_data_t {
    struct str_t str;
    size_t id;
};

struct code_gen_t {
    struct scope_t root_scope;
    struct error_message_t** errors;
    size_t error_line;
    size_t error_column;
    enum error_color_t error_color;
    struct allocator_t* allocator;
    struct string_data_t* data_strings;
};

void analyse(
    struct code_gen_t code_gen[static const 1],
    struct ast_node_t const root[static const 1]
);

void generate(
    struct code_gen_t code_gen[static const 1],
    struct str_buffer_t buffer[static const 1],
    struct ast_node_t const root[static const 1]
);
