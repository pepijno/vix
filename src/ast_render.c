#include "ast_render.h"

#include "parser.h"
#include "util.h"

typedef struct {
    size_t const indent;
    FILE* const f;
} ast_print_t;

static char* node_type_string(node_type_e type) {
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
        case NODE_TYPE_FREE_OBJECT_COPY_PARAMS:
            return "FreeObjectCopyParams";
        case NODE_TYPE_OBJECT_COPY:
            return "ObjectCopy";
        case NODE_TYPE_DECORATOR:
            return "Decorator";
        case NODE_TYPE_ROOT:
            return "Root";
    }
    vix_unreachable();
}

static void ast_print_visit(ast_node_t* node, void* context) {
    ast_print_t* ast_print = (ast_print_t*) context;

    for (size_t i = 0; i < ast_print->indent; ++i) {
        fprintf(ast_print->f, " ");
    }

    fprintf(ast_print->f, "%ld %s ", node->id, node_type_string(node->type));
    if (node->type == NODE_TYPE_IDENTIFIER) {
        fprintf(
            ast_print->f, "\"" str_fmt "\"",
            str_args(node->data.identifier.content)
        );
    } else if (node->type == NODE_TYPE_STRING_LITERAL) {
        fprintf(
            ast_print->f, "\"" str_fmt "\"",
            str_args(node->data.string_literal.content)
        );
    } else if (node->type == NODE_TYPE_CHAR_LITERAL) {
        fprintf(ast_print->f, "'%c'", node->data.char_literal.c);
    } else if (node->type == NODE_TYPE_INTEGER) {
        fprintf(
            ast_print->f, "\"" str_fmt "\"",
            str_args(node->data.integer.content)
        );
    }
    fprintf(ast_print->f, "\n");

    ast_print_t new_ast_print = {
        .f      = ast_print->f,
        .indent = ast_print->indent + 2
    };
    ast_visit_node_children(node, ast_print_visit, &new_ast_print);
}

void ast_print(FILE* f, ast_node_t* node, size_t indent) {
    ast_print_t ast_print = {.f = f, .indent = indent};
    ast_visit_node_children(node, ast_print_visit, &ast_print);
}
