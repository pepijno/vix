#pragma once

#include "error_message.h"
#include "lexer.h"

#include <stdint.h>

typedef enum {
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
} node_type_e;

typedef struct ast_node_t ast_node_t;

typedef struct {
    str_t content;
} ast_string_literal_t;

typedef struct {
    uint8_t c;
} ast_char_literal_t;

typedef struct {
    str_t content;
} ast_integer_t;

typedef struct {
    str_t content;
} ast_identifier_t;

typedef struct {
    ast_node_t** free_list;
    ast_node_t** property_list;
    ast_node_t* object_copy;
} ast_object_t;

typedef struct {
    ast_node_t* identifier;
    ast_node_t* property_value;
} ast_property_t;

typedef struct {
    ast_node_t* identifier;
    ast_node_t** free_object_copies;
    ast_node_t* object_copy;
} ast_object_copy_t;

typedef struct {
    ast_node_t** parameter_list;
} ast_free_object_copy_params_t;

typedef struct {
    ast_node_t* object;
} ast_decorator_t;

typedef struct {
    ast_node_t** list;
} ast_root_t;

typedef struct {
    ast_node_t* root;
    str_t path;
    str_t source_code;
    size_t* line_offsets;
} import_table_entry_t;

struct ast_node_t {
    size_t id;
    node_type_e const type;
    ast_node_t* parent;
    size_t const line;
    size_t const column;
    import_table_entry_t* owner;
    union {
        ast_string_literal_t string_literal;
        ast_char_literal_t char_literal;
        ast_integer_t integer;
        ast_object_t object;
        ast_property_t property;
        ast_identifier_t identifier;
        ast_object_copy_t object_copy;
        ast_free_object_copy_params_t free_object_copy_params;
        ast_decorator_t decorator;
        ast_root_t root;
    } data;
};

void ast_visit_node_children(
    ast_node_t node[static 1], void (*visit)(ast_node_t*, void* context),
    void* context
);
ast_node_t* ast_parse(
    allocator_t allocator[static 1], str_t source,
    token_ptr_array_t tokens[static 1],
    import_table_entry_t owner[static 1], error_color_e error_color
);
