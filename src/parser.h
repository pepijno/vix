#pragma once

#include "lexer.h"

struct ast_free_property_t {
    char* name;
    struct ast_free_property_t* next;
};

struct ast_property_t;
struct ast_object_copy_t;

enum object_type_e {
    OBJECT_TYPE_NONE,
    OBJECT_TYPE_OBJECT_COPY,
    OBJECT_TYPE_PROPERTIES,
    OBJECT_TYPE_INTEGER,
    OBJECT_TYPE_STRING
};

struct ast_object_t {
    enum object_type_e type;
    struct ast_free_property_t* free_properties;
    union {
        struct ast_object_copy_t* object_copy;
        struct ast_property_t* properties;
        i32 integer;
        char* string;
    };
};

struct ast_free_property_assign_t {
    struct ast_object_t* value;
    struct ast_free_property_assign_t* next;
};

struct ast_object_copy_t {
    char* name;
    struct ast_free_property_assign_t* free_properties;
    struct ast_object_copy_t* next;
};

struct ast_property_t {
    char* name;
    struct ast_object_t* object;
    struct ast_property_t* next;
};

struct ast_object_t* parse(struct lexer_t lexer[static 1]);
void print_object(struct ast_object_t* object, i32 indent);
