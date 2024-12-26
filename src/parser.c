#include "parser.h"

#include "allocator.h"
#include "ast.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdnoreturn.h>

VECTOR_IMPL(struct ast_property*, ast_property_ptr)

static noreturn void
vsynerror(
    struct allocator allocator[static const 1], struct token token, va_list ap
) {
    enum lex_token_type type = va_arg(ap, enum lex_token_type);

    fprintf(
        stderr, STR_FMT ":%d:%d: syntax error: expected ",
        STR_ARG(sources[token.location.file]), token.location.line_number,
        token.location.column_number
    );

    while (type != TOKEN_EOF) {
        if (type == TOKEN_INTEGER || type == TOKEN_NAME) {
            fprintf(stderr, STR_FMT, STR_ARG(token_names[type]));
        } else {
            fprintf(stderr, "'" STR_FMT "'", STR_ARG(token_names[type]));
        }

        type = va_arg(ap, enum lex_token_type);
        fprintf(stderr, ", ");
    }

    fprintf(stderr, "found '" STR_FMT "'\n", STR_ARG(token_names[token.type]));
    error_line(allocator, token.location);
    exit(EXIT_PARSE);
}

static noreturn void
synerror(struct allocator allocator[static const 1], struct token token, ...) {
    va_list ap;
    va_start(ap, token);
    vsynerror(allocator, token, ap);
    va_end(ap);
}

static void
synassert(
    struct allocator allocator[static const 1], bool condition,
    struct token token, ...
) {
    if (!condition) {
        va_list ap;
        va_start(ap, token);
        vsynerror(allocator, token, ap);
        va_end(ap);
    }
}

static void
expect_token(
    struct lexer lexer[static const 1], enum lex_token_type token_type,
    struct token token[const 1]
) {
    struct token _token = {};
    struct token* out   = token != nullptr ? token : &_token;
    lex(lexer, out);
    synassert(
        lexer->allocator, out->type == token_type, *out, token_type, TOKEN_EOF
    );
    if (token == nullptr) {
        token_finish(out);
    }
}

static struct ast_property* parse_property(
    struct parser_context parser_context[static const 1],
    struct lexer lexer[static const 1]
);

static struct ast_element*
parse_element(
    struct parser_context parser_context[static const 1],
    struct lexer lexer[static const 1]
) {
    struct token token          = {};
    struct ast_element* element = allocator_allocate(
        parser_context->allocator, sizeof(struct ast_element)
    );

    switch (lex(lexer, &token)) {
        case TOKEN_NAME:
            element->type = AST_ELEMENT_TYPE_ID;
            element->id.id
                = string_duplicate(parser_context->allocator, token.name);
            break;
        case TOKEN_OPEN_BRACE:
            struct token token2 = {};
            bool break_out      = false;
            element->type       = AST_ELEMENT_TYPE_PROPERTIES;
            element->properties
                = vector_ast_property_ptr_new(parser_context->allocator);
            while (!break_out) {
                switch (lex(lexer, &token2)) {
                    case TOKEN_CLOSE_BRACE:
                        break_out = true;
                        break;
                    default:
                        unlex(lexer, &token2);
                        struct ast_property* next
                            = parse_property(parser_context, lexer);
                        vector_ast_property_ptr_add(&element->properties, next);
                        break;
                }
            }
            break;
        case TOKEN_INTEGER:
            element->type          = AST_ELEMENT_TYPE_INTEGER;
            element->integer.value = token.integer;
            break;
        case TOKEN_STRING:
            element->type = AST_ELEMENT_TYPE_STRING;
            element->string.value
                = string_duplicate(parser_context->allocator, token.string);
            break;
        default:
            unlex(lexer, &token);
            break;
    }

    return element;
}

static struct ast_property*
parse_property(
    struct parser_context parser_context[static const 1],
    struct lexer lexer[static const 1]
) {
    struct token token       = {};
    enum lex_token_type type = lex(lexer, &token);
    if (type != TOKEN_NAME) {
        unlex(lexer, &token);
        return nullptr;
    }
    struct ast_property* property = allocator_allocate(
        parser_context->allocator, sizeof(struct ast_property)
    );
    property->id = parser_context->next_id;
    parser_context->next_id += 1;
    property->name = string_duplicate(parser_context->allocator, token.name);
    expect_token(lexer, TOKEN_ASSIGN, nullptr);
    property->value = parse_element(parser_context, lexer);
    expect_token(lexer, TOKEN_SEMICOLON, nullptr);

    hashmap_properties_add(&parser_context->properties, property->id, property);

    return property;
}

static struct ast_element*
parse_root(
    struct parser_context parser_context[static const 1],
    struct lexer lexer[static const 1]
) {
    struct ast_element* root = allocator_allocate(
        parser_context->allocator, sizeof(struct ast_element)
    );
    root->type       = AST_ELEMENT_TYPE_PROPERTIES;
    root->properties = vector_ast_property_ptr_new(parser_context->allocator);

    for (;;) {
        struct ast_property* property = parse_property(parser_context, lexer);
        if (property == nullptr) {
            break;
        }
        vector_ast_property_ptr_add(&root->properties, property);
    }

    return root;
}

struct ast_element*
parse(
    struct parser_context parser_context[static const 1],
    struct lexer lexer[static const 1]
) {
    struct ast_element* root = parse_root(parser_context, lexer);
    expect_token(lexer, TOKEN_EOF, nullptr);
    return root;
}
