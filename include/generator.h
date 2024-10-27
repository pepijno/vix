#pragma once

#include "defs.h"
#include "str.h"
enum generated_value_type {
    GENERATED_VALUE_TYPE_CONSTANT,
    GENERATED_VALUE_TYPE_GLOBAL,
    GENERATED_VALUE_TYPE_TEMPORARY,
};

struct generated_value {
    enum generated_value_type generated_value_type;
    union {
        struct string name;
        u32 u32;
        u64 u64;
        f32 f32;
        f64 f64;
    };
};

struct qbe_program;
struct ast_object;
struct function_type;

void generate(
    struct arena* arena, struct qbe_program* program, struct ast_object* root,
    struct function_type* root_type
);
