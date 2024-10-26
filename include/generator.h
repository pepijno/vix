#pragma once

#include "analyser.h"
#include "parser.h"
#include "qbe.h"

enum generated_value_type {
    GENERATED_VALUE_TYPE_CONSTANT,
    GENERATED_VALUE_TYPE_GLOBAL,
    GENERATED_VALUE_TYPE_TEMPORARY,
};

struct generated_value {
    enum generated_value_type generated_value_type;
    union {
        char *name;
        u32 u32;
        u64 u64;
        f32 f32;
        f64 f64;
    };
};

void generate(
    struct qbe_program program[static 1], struct ast_object root[static 1],
    struct function_type root_type[static 1]
);
