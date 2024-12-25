#pragma once

#include "defs.h"
#include "str.h"
#include "vector.h"

struct instruction_push_int {
    u64 value;
};

struct instruction_push_str {
    struct string value;
};

struct instruction_push_global {
    struct string name;
};

struct instruction_push {
    usize offset;
};

struct instruction_pack {
    usize size;
    u8 tag;
};

struct instruction_split {
    usize size;
};

enum instruction_type {
    INSTRUCTION_TYPE_PUSH_INT,
    INSTRUCTION_TYPE_PUSH_STR,
    INSTRUCTION_TYPE_PUSH_GLOBAL,
    INSTRUCTION_TYPE_PUSH,
    INSTRUCTION_TYPE_PACK,
    INSTRUCTION_TYPE_SPLIT,
};

struct instruction {
    enum instruction_type type;
    union {
        struct instruction_push_int push_int;
        struct instruction_push_str push_str;
        struct instruction_push_global push_global;
        struct instruction_push push;
        struct instruction_pack pack;
        struct instruction_split split;
    };
};

VECTOR_DEFS(struct instruction, instruction)

struct compilation_env;
struct _ast_element;

void compile(
    struct arena* arena, struct compilation_env compilation_env,
    struct _ast_element element, struct vector_instruction* instructions
);
void _emit(struct vector_instruction instructions);
