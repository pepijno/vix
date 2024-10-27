#pragma once

#include "defs.h"

struct ast_free_property;
struct ast_property;
struct ast_object_copy;

enum ast_object_alias {
    AST_OBJECT_ALIAS_NONE,
    AST_OBJECT_ALIAS_INTEGER,
    AST_OBJECT_ALIAS_STRING,
    AST_OBJECT_ALIAS_OBJECT_COPY,
    AST_OBJECT_ALIAS_ANY,
};

struct ast_object {
    u32 id;
    enum ast_object_alias alias;
    struct ast_property* properties;
    union {
        struct ast_object_copy* object_copy;
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

enum ast_property_type {
    AST_PROPERTY_TYPE_FREE,
    AST_PROPERTY_TYPE_NON_FREE,
};

struct ast_property {
    u32 id;
    enum ast_property_type type;
    struct string name;
    struct ast_object* object;
    struct ast_property* next;
};

struct arena;
struct lexer;

struct ast_object* parse(struct arena* arena, struct lexer* lexer);
void print_object(struct ast_object* object, usize indent);
