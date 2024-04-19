#pragma once

#include "error_message.h"
#include "lexer.h"

#include <stdint.h>

enum node_type_t {
    NODE_TYPE_STRING_LITERAL,
    NODE_TYPE_CHAR_LITERAL,
    NODE_TYPE_INTEGER,
    NODE_TYPE_OBJECT,
    NODE_TYPE_PROPERTY,
    NODE_TYPE_IDENTIFIER,
    NODE_TYPE_FREE_OBJECT_COPY_PARAMS,
    NODE_TYPE_OBJECT_COPY,
    NODE_TYPE_DECORATOR,
    NODE_TYPE_ROOT
};

struct ast_node_t;

struct ast_string_literal_t {
    struct str_t content;
};

struct ast_char_literal_t {
    uint8_t c;
};

struct ast_integer_t {
    struct str_t content;
};

struct ast_identifier_t {
    struct str_t content;
};

struct ast_object_t {
    struct ast_node_t** free_list;
    struct ast_node_t** property_list;
    struct ast_node_t* object_copy;
};

struct ast_property_t {
    struct ast_node_t* identifier;
    struct ast_node_t* property_value;
};

struct ast_object_copy_t {
    struct ast_node_t* identifier;
    struct ast_node_t** free_object_copies;
    struct ast_node_t* object_copy;
};

struct ast_free_object_copy_params_t {
    struct ast_node_t** parameter_list;
};

struct ast_decorator_t {
    struct ast_node_t* object;
};

struct ast_root_t {
    struct ast_node_t** list;
};

struct import_table_entry_t;

struct ast_node_t {
    size_t id;
    enum node_type_t const type;
    struct ast_node_t* parent;
    size_t const line;
    size_t const column;
    struct import_table_entry_t* owner;
    union {
        struct ast_string_literal_t string_literal;
        struct ast_char_literal_t char_literal;
        struct ast_integer_t integer;
        struct ast_object_t object;
        struct ast_property_t property;
        struct ast_identifier_t identifier;
        struct ast_object_copy_t object_copy;
        struct ast_free_object_copy_params_t free_object_copy_params;
        struct ast_decorator_t decorator;
        struct ast_root_t root;
    } data;
};

struct import_table_entry_t {
    struct ast_node_t* root;
    struct str_t path;
    struct str_t source_code;
    size_t* line_offsets;
};

void ast_visit_node_children(
    struct ast_node_t node[static const 1],
    void (*visit)(struct ast_node_t*, void* context), void* context
);
struct ast_node_t* ast_parse(
    struct allocator_t allocator[static const 1], struct str_t source,
    token_ptr_array_t tokens[static const 1],
    struct import_table_entry_t owner[static const 1],
    enum error_color_t const error_color
);
