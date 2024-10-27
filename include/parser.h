#pragma once

#include "defs.h"
#include "str.h"

struct ast_free_property;
struct ast_property;
struct ast_object_copy;

enum object_type {
    OBJECT_TYPE_NONE,
    OBJECT_TYPE_OBJECT_COPY,
    OBJECT_TYPE_PROPERTIES,
    OBJECT_TYPE_INTEGER,
    OBJECT_TYPE_STRING
};

struct ast_object {
    u32 id;
    struct ast_object* parent;
    enum object_type type;
    struct ast_free_property* free_properties;
    union {
        struct ast_object_copy* object_copy;
        struct ast_property* properties;
        u64 integer;
        struct string string;
    };
};

struct ast_free_property_assign {
    struct ast_object* value;
    struct ast_free_property_assign* next;
};

struct ast_object_copy {
    struct string name;
    struct ast_free_property_assign* free_properties;
    struct ast_object_copy* next;
};

struct ast_free_property {
    u32 id;
    struct string name;
    struct ast_free_property* next;
};

struct ast_property {
    struct string name;
    struct ast_object* object;
    struct ast_property* next;
};

struct arena;
struct lexer;

struct ast_object* parse(struct arena* arena, struct lexer* lexer);
void print_object(struct ast_object* object, usize indent);
