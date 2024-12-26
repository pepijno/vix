#pragma once

#include "defs.h"
#include "str.h"
#include "vector.h"

VECTOR_DEFS(struct string, string)

struct ast_element;

struct ast_integer {
    u64 value;
};

struct ast_string {
    struct string value;
};

struct ast_property_access {
    struct vector_string ids;
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
    AST_ELEMENT_TYPE_PROPERTIES,
    AST_ELEMENT_TYPE_PROPERTY_ACCESS,
};

struct ast_element {
    enum ast_element_type type;
    struct type_env* env;
    union {
        struct ast_integer integer;
        struct ast_string string;
        struct ast_property_access property_access;
        struct vector_ast_property_ptr properties;
    };
};
