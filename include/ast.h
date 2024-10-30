#pragma once

#include "defs.h"

struct ast_type;

struct ast_object_property_type {
    struct string name;
    struct ast_type* type;
    struct ast_object_property_type* next;
};

struct ast_union_type {
    struct ast_type* type;
    struct ast_union_type* next;
};

enum ast_stype {
    AST_STYPE_ANY,
    AST_STYPE_COPY,
    AST_STYPE_OBJECT,
    AST_STYPE_UNION,
    AST_STYPE_ALIAS,
};

enum ast_extra_stype {
    AST_EXTRA_STYPE_NONE,
    AST_EXTRA_STYPE_INTEGER,
    AST_EXTRA_STYPE_STRING,
};

struct ast_type {
    enum ast_stype type;
    enum ast_extra_stype extra_type;
    union {
        struct ast_object_property_type* object_types;
        struct ast_union_type* union_type;
        u32 alias_id;
    };
};

struct ast_property;
struct ast_object_copy;

struct ast_object {
    u32 id;
    struct ast_object* parent;
    struct ast_type type;
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
