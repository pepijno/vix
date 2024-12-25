#pragma once

#include "defs.h"
#include "str.h"
#include "types.h"
#include "vector.h"

struct _ast_element;

struct _ast_id {
    struct string id;
};

struct _ast_integer {
    u64 value;
};

struct _ast_string {
    struct string value;
};

struct type;

struct _ast_property {
    usize id;
    struct string name;
    struct type* type;
    struct _ast_element* value;
};

VECTOR_DEFS(struct _ast_property*, ast_property_ptr)

enum _ast_element_type {
    AST_ELEMENT_TYPE_INTEGER,
    AST_ELEMENT_TYPE_STRING,
    AST_ELEMENT_TYPE_ID,
    AST_ELEMENT_TYPE_PROPERTIES,
};

struct _ast_element {
    enum _ast_element_type type;
    struct type_env* env;
    union {
        struct _ast_id id;
        struct _ast_integer integer;
        struct _ast_string string;
        struct vector_ast_property_ptr properties;
    };
};
