#include "ast_render.h"

#include "parser.h"
#include "util.h"

typedef struct {
    i64 const indent;
    FILE* const f;
} AstPrint;

static Str node_type_string(NodeType type) {
    switch (type) {
        case NODE_TYPE_STRING_LITERAL:
            return str_new("StringLiteral");
        case NODE_TYPE_CHAR_LITERAL:
            return str_new("CharLiteral");
        case NODE_TYPE_INTEGER:
            return str_new("Integer");
        case NODE_TYPE_OBJECT:
            return str_new("Object");
        case NODE_TYPE_PROPERTY:
            return str_new("Property");
        case NODE_TYPE_IDENTIFIER:
            return str_new("Identifier");
        case NODE_TYPE_FREE_OBJECT_COPY_PARAMS:
            return str_new("FreeObjectCopyParams");
        case NODE_TYPE_OBJECT_COPY:
            return str_new("ObjectCopy");
        case NODE_TYPE_DECORATOR:
            return str_new("Decorator");
        case NODE_TYPE_ROOT:
            return str_new("Root");
    }
    vix_unreachable();
}

static void ast_print_visit(AstNode* node, void* context) {
    AstPrint* ast_print = (AstPrint*) context;

    for (i64 i = 0; i < ast_print->indent; ++i) {
        fprintf(ast_print->f, " ");
    }

    fprintf(
        ast_print->f, "%ld " str_fmt " ", node->id,
        str_args(node_type_string(node->type))
    );
    if (node->type == NODE_TYPE_IDENTIFIER) {
        fprintf(
            ast_print->f, "\"" str_fmt "\"", str_args(node->identifier.content)
        );
    } else if (node->type == NODE_TYPE_STRING_LITERAL) {
        fprintf(
            ast_print->f, "\"" str_fmt "\"",
            str_args(node->string_literal.content)
        );
    } else if (node->type == NODE_TYPE_CHAR_LITERAL) {
        fprintf(ast_print->f, "'%c'", node->char_literal.c);
    } else if (node->type == NODE_TYPE_INTEGER) {
        fprintf(
            ast_print->f, "\"" str_fmt "\"", str_args(node->integer.content)
        );
    }
    fprintf(ast_print->f, "\n");

    AstPrint new_ast_print = {
        .f      = ast_print->f,
        .indent = ast_print->indent + 2
    };
    ast_visit_node_children(node, ast_print_visit, &new_ast_print);
}

void ast_print(FILE* f, AstNode* node, i64 indent) {
    AstPrint ast_print = {.f = f, .indent = indent};
    ast_visit_node_children(node, ast_print_visit, &ast_print);
}
