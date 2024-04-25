#include "ast_render.h"

#include "parser.h"
#include "util.h"

typedef struct {
    i64 const indent;
    FILE* const f;
} AstPrint;

static u8* node_type_string(NodeType type) {
    switch (type) {
        case NODE_TYPE_STRING_LITERAL:
            return (u8*)"StringLiteral";
        case NODE_TYPE_CHAR_LITERAL:
            return (u8*)"CharLiteral";
        case NODE_TYPE_INTEGER:
            return (u8*)"Integer";
        case NODE_TYPE_OBJECT:
            return (u8*)"Object";
        case NODE_TYPE_PROPERTY:
            return (u8*)"Property";
        case NODE_TYPE_IDENTIFIER:
            return (u8*)"Identifier";
        case NODE_TYPE_FREE_OBJECT_COPY_PARAMS:
            return (u8*)"FreeObjectCopyParams";
        case NODE_TYPE_OBJECT_COPY:
            return (u8*)"ObjectCopy";
        case NODE_TYPE_DECORATOR:
            return (u8*)"Decorator";
        case NODE_TYPE_ROOT:
            return (u8*)"Root";
    }
    vix_unreachable();
}

static void ast_print_visit(AstNode* node, void* context) {
    AstPrint* ast_print = (AstPrint*) context;

    for (i64 i = 0; i < ast_print->indent; ++i) {
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
