#pragma once

#include "defs.h"
#include "str.h"
#include "vector.h"

struct ast_element;

struct ast_id {
    struct string id;
};

struct ast_integer {
    u64 value;
};

struct ast_string {
    struct string value;
};

struct type;

struct ast_property {
    usize id;
    struct string name;
    struct type* type;
    struct ast_element* value;
};

VECTOR_DEFS(struct ast_property*, ast_property_ptr)

enum ast_element_type {
    AST_ELEMENT_TYPE_INTEGER,
    AST_ELEMENT_TYPE_STRING,
    AST_ELEMENT_TYPE_ID,
    AST_ELEMENT_TYPE_PROPERTIES,
};

struct ast_element {
    enum ast_element_type type;
    struct type_env* env;
    union {
        struct ast_id id;
        struct ast_integer integer;
        struct ast_string string;
        struct vector_ast_property_ptr properties;
    };
};
