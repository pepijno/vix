#pragma once

#include "parser.h"

struct function_type;

enum return_type {
    RETURN_INTEGER,
    RETURN_STRING,
    RETURN_OBJECT,
    RETURN_COPY,
    RETURN_ANY,
    RETURN_UNION,
};

struct function_param_type {
    char* name;
    struct function_type* function_type;
    struct function_param_type* next;
};

struct return_object_property {
    char* name;
    struct function_type* function_type;
    struct return_object_property* next;
};

struct function_union {
    struct function_type* type;
    struct function_union* next;
};

struct function_return_type {
    enum return_type return_type;
    union {
        struct return_object_property* properties;
        struct function_type* copy_type;
        struct function_union* function_union;
    };
};

struct function_type {
    i32 id;
    struct function_return_type return_type;
    struct function_param_type* param_types;
};

void analyse(struct ast_object root[static 1]);
