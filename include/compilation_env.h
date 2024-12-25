#pragma once

#include "defs.h"
#include "str.h"

struct compilation_env_var {
    struct string name;
};

struct compilation_env_offset {
    usize offset;
};

enum compilation_env_type {
    COMPILATION_ENV_TYPE_VAR,
    COMPILATION_ENV_TYPE_OFFSET,
};

struct compilation_env {
    enum compilation_env_type type;
    struct compilation_env* parent;
    union {
        struct compilation_env_var env_var;
        struct compilation_env_offset env_offset;
    };
};

bool has_variable(struct compilation_env compilation_env, struct string name);
usize get_offset(struct compilation_env compilation_env, struct string name);
