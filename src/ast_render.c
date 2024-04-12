#include "ast_render.h"

#include "parser.h"
#include "str.h"
#include "util.h"

struct ast_print_t {
    size_t const indent;
    FILE* const f;
};

static char* node_type_string(enum node_type_t const type) {
    switch (type) {
    case NODE_TYPE_STRING_LITERAL:
        return "StringLiteral";
    case NODE_TYPE_CHAR_LITERAL:
        return "CharLiteral";
    case NODE_TYPE_INTEGER:
        return "Integer";
    case NODE_TYPE_OBJECT:
        return "Object";
    case NODE_TYPE_PROPERTY:
        return "Property";
    case NODE_TYPE_IDENTIFIER:
        return "Identifier";
    case NODE_TYPE_FREE_OBJECT_COPY:
        return "FreeObjectCopy";
    case NODE_TYPE_OBJECT_PROPERTY_ACCESS:
        return "ObjectPropertyAccess";
    case NODE_TYPE_DECORATOR:
        return "Decorator";
    case NODE_TYPE_ROOT:
        return "Root";
    }
    vix_unreachable();
}

static void ast_print_visit(struct ast_node_t* node, void* context) {
    struct ast_print_t const* const ast_print = (struct ast_print_t*) context;

    for (size_t i = 0; i < ast_print->indent; ++i) {
        fprintf(ast_print->f, " ");
    }

    fprintf(ast_print->f, "%s ", node_type_string(node->type));
    if (node->type == NODE_TYPE_IDENTIFIER) {
        fprintf(ast_print->f, "\"%s\"", node->data.identifier.content);
    } else if (node->type == NODE_TYPE_STRING_LITERAL) {
        fprintf(ast_print->f, "\"%s\"", node->data.string_literal.content);
    } else if (node->type == NODE_TYPE_CHAR_LITERAL) {
        fprintf(ast_print->f, "'%c'", node->data.char_literal.c);
    } else if (node->type == NODE_TYPE_INTEGER) {
        fprintf(ast_print->f, "\"%s\"", node->data.integer.content);
    }
    fprintf(ast_print->f, "\n");

    struct ast_print_t new_ast_print = {
        .f      = ast_print->f,
        .indent = ast_print->indent + 2
    };
    ast_visit_node_children(node, ast_print_visit, &new_ast_print);
}

void ast_print(FILE* const f, struct ast_node_t* node, size_t const indent) {
    struct ast_print_t ast_print = {.f = f, .indent = indent};
    ast_visit_node_children(node, ast_print_visit, &ast_print);
}
