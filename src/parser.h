#pragma once

#include "lexer.h"

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
    i32 id;
    struct ast_object* parent;
    enum object_type type;
    struct ast_free_property* free_properties;
    union {
        struct ast_object_copy* object_copy;
        struct ast_property* properties;
        i32 integer;
        char* string;
    };
};

struct ast_free_property_assign {
    struct ast_object* value;
    struct ast_free_property_assign* next;
};

struct ast_object_copy {
    char* name;
    struct ast_free_property_assign* free_properties;
    struct ast_object_copy* next;
};

struct ast_free_property {
    char* name;
    struct ast_free_property* next;
};

struct ast_property {
    char* name;
    struct ast_object* object;
    struct ast_property* next;
};

struct ast_object* parse(struct lexer lexer[static 1]);
void print_object(struct ast_object* object, i32 indent);
