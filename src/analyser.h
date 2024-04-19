#pragma once

#include "allocator.h"
#include "parser.h"

typedef struct scope_t scope_t;
struct scope_t {
    ast_node_t const* const source_node;
    scope_t* parent;
    ast_node_t** properties;
};

typedef struct {
    str_t str;
    size_t id;
} string_data_t;

typedef struct {
    scope_t root_scope;
    error_message_t** errors;
    size_t error_line;
    size_t error_column;
    error_color_e error_color;
    allocator_t* allocator;
    string_data_t* data_strings;
} code_gen_t;

void analyse(
    code_gen_t code_gen[static 1],
    ast_node_t root[static 1]
);

void generate(
    code_gen_t code_gen[static 1],
    str_buffer_t buffer[static 1],
    ast_node_t root[static 1]
);
