#pragma once

#include "hashmap.h"
#include "lexer.h"

struct allocator;
struct ast_element;

HASHMAP_DEFS(usize, struct ast_property*, properties)

struct parser_context {
    struct allocator* allocator;
    usize next_id;
    struct hashmap_properties properties;
};

struct ast_element* parse(
    struct parser_context parser_context[static const 1],
    struct lexer lexer[static const 1]
);
