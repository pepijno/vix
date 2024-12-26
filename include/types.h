#pragma once

#include "ast.h"
#include "defs.h"
#include "graph.h"
#include "hashmap.h"
#include "instructions.h"
#include "vector.h"

struct type_var {
    struct string name;
};

struct type_base {
    struct string name;
};

struct type;

struct type_arrow {
    struct type* left;
    struct type* right;
};

struct type_property {
    struct string name;
    struct type* type;
};

VECTOR_DEFS(struct type_property*, type_property_ptr)

enum type_type {
    TYPE_TYPE_VAR,
    TYPE_TYPE_BASE,
    TYPE_TYPE_ARROW,
    TYPE_TYPE_PROPERTIES,
};

struct type {
    enum type_type type;
    union {
        struct type_var var;
        struct type_base base;
        struct type_arrow arrow;
        struct vector_type_property_ptr properties;
    };
};

HASHMAP_DEFS(struct string, struct type*, string_type)

struct type_context {
    usize last_id;
    struct allocator* allocator;
    struct hashmap_string_type types;
};

struct type_env {
    struct allocator* allocator;
    struct hashmap_string_type names;
    struct type_env* parent;
};

HASHMAP_DEFS(struct string, usize, string_id)

struct typecheck_env {
    struct typecheck_env* parent;
    struct hashmap_string_id names;
};

void env_bind(
    struct type_env type_env[static const 1], struct string const name,
    struct type type[static const 1]
);
struct type* typecheck(
    struct type_context context[static const 1],
    struct type_env env[static const 1],
    struct ast_element element[static const 1]
);
void typecheck_properties_first(
    struct type_context context[static const 1],
    struct type_env env[static const 1], struct string const name,
    struct ast_element element[static const 1]
);
void typecheck_properties_second(
    struct type_context context[static const 1],
    struct type_env env[static const 1],
    struct ast_element element[static const 1]
);

struct vector_ast_property_ptr;

void typecheck_init_properties(
    struct allocator allocator[static const 1],
    struct object_graph object_graph[static const 1],
    struct vector_ast_property_ptr properties,
    struct typecheck_env env[static const 1]
);
