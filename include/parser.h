#pragma once

#include "hashmap.h"

struct arena;
struct lexer;
struct _ast_element;

HASHMAP_DEFS(usize, struct _ast_property*, properties)

struct parser_context {
    struct arena* arena;
    usize next_id;
    struct hashmap_properties properties;
};

struct _ast_element* _parse(struct parser_context* parser_context, struct lexer* lexer);
