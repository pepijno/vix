#pragma once

#include "defs.h"
#include "error_message.h"
#include "lexer.h"

typedef enum {
    NODE_TYPE_STRING_LITERAL,
    NODE_TYPE_CHAR_LITERAL,
    NODE_TYPE_INTEGER,
    NODE_TYPE_OBJECT,
    NODE_TYPE_PROPERTY,
    NODE_TYPE_IDENTIFIER,
    NODE_TYPE_FREE_OBJECT_COPY_PARAMS,
    NODE_TYPE_OBJECT_COPY,
    NODE_TYPE_DECORATOR,
    NODE_TYPE_ROOT
} NodeType;

typedef struct AstNode AstNode;

typedef struct {
    Str content;
} AstStringLiteral;

typedef struct {
    u8 c;
} AstCharLiteral;

typedef struct {
    Str content;
} AstInteger;

typedef struct {
    Str content;
} AstIdentifier;

typedef struct {
    AstNode** free_list;
    AstNode** property_list;
    AstNode* object_copy;
} AstObject;

typedef struct {
    AstNode* identifier;
    AstNode* property_value;
} AstProperty;

typedef struct {
    AstNode* identifier;
    AstNode** free_object_copies;
    AstNode* object_copy;
} AstObjectCopy;

typedef struct {
    AstNode** parameter_list;
} AstFreeObjectCopyParams;

typedef struct {
    AstNode* object;
} AstDecorator;

typedef struct {
    AstNode** list;
} AstRoot;

typedef struct {
    AstNode* root;
    Str path;
    Str source_code;
    i64* line_offsets;
} ImportTableEntry;

struct AstNode {
    i64 id;
    NodeType const type;
    AstNode* parent;
    i64 const line;
    i64 const column;
    ImportTableEntry* owner;
    union {
        AstStringLiteral string_literal;
        AstCharLiteral char_literal;
        AstInteger integer;
        AstObject object;
        AstProperty property;
        AstIdentifier identifier;
        AstObjectCopy object_copy;
        AstFreeObjectCopyParams free_object_copy_params;
        AstDecorator decorator;
        AstRoot root;
    } data;
};

void ast_visit_node_children(
    AstNode node[static 1], void (*visit)(AstNode*, void* context),
    void* context
);
AstNode* ast_parse(
    Allocator allocator[static 1], Str source,
    TokenPtrArray tokens[static 1], ImportTableEntry owner[static 1],
    ErrorColor error_color
);
