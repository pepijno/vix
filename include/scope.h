#pragma once

#include "defs.h"

#define BUCKET_SIZE 4096

struct ast_object;
struct ast_property;

enum scope_object_type {
    SCOPE_OBJECT_TYPE_OBJECT,
    SCOPE_OBJECT_TYPE_PROPERTY,
};

struct scope_object {
    enum scope_object_type type;
    union {
        struct ast_object* object;
        struct ast_property* property;
    };
    struct scope_object* lnext;
    struct scope_object* mnext;
};

struct scope {
    struct scope* parent;

    struct scope_object* objects;
    struct scope_object** next;

    struct scope_object* buckets[BUCKET_SIZE];
};

struct arena;

struct scope* scope_push(struct arena* arena, struct scope** stack);
struct scope* scope_pop(struct scope** stack);
void scope_insert_property(
    struct arena* arena, struct scope* scope, struct ast_property* property
);
struct scope_object* scope_lookup_name(
    struct scope* scope, struct string const name
);
