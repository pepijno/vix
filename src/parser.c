#include "parser.h"

#include "array.h"
#include "lexer.h"
#include "str.h"
#include "util.h"

#include <assert.h>
#include <string.h>

struct parse_context_t {
    char const* const source;
    struct ast_node_t* root;
    token_ptr_array_t* tokens;
    enum error_color_t const error_color;
    struct import_table_entry_t* owner;
    struct allocator_t* allocator;
};

static void ast_error(
    struct parse_context_t const parse_context[static const 1],
    struct token_t const token[static const 1], char const* const message
) {
    struct error_message_t const error = error_message_create_with_line(
        parse_context->allocator, parse_context->owner->path, token->start_line,
        token->start_column, parse_context->owner->source_code,
        parse_context->owner->line_offsets, message
    );

    print_error_message(&error, parse_context->error_color);
    exit(EXIT_FAILURE);
}

static char* token_buffer(struct token_t const token[static const 1]) {
    assert(
        token->type == TOKEN_STRING_LITERAL || token->type == TOKEN_IDENTIFIER
    );
    return str_copy(token->data.string_literal.string);
}

static void ast_expect_token(
    struct parse_context_t const parse_context[static const 1],
    struct token_t const token[static const 1], enum token_type_t const type
) {
    if (token->type == type) {
        return;
    }

    ast_error(
        parse_context, token,
        str_printf(
            parse_context->allocator, "expected token '%s', found '%s'",
            token_name(type), token_name(token->type)
        )
    );
}

static struct token_t* ast_eat_token(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1], enum token_type_t const type
) {
    struct token_t* const token = (*parse_context->tokens)[*token_index];
    ast_expect_token(parse_context, token, type);
    *token_index += 1;
    return token;
}

static struct ast_node_t* ast_node_create(
    struct parse_context_t const parse_context[static const 1],
    enum node_type_t const type,
    struct token_t const first_token[static const 1]
) {
    struct ast_node_t node = {
        .type   = type,
        .line   = first_token->start_line,
        .column = first_token->start_column,
        .owner  = parse_context->owner,
    };
    struct ast_node_t* const ptr =
        (struct ast_node_t*) parse_context->allocator->allocate(
            sizeof(struct ast_node_t), parse_context->allocator->context
        );
    memcpy(ptr, &node, sizeof(struct ast_node_t));
    return ptr;
}

static struct ast_node_t* ast_parse_property(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
);
static struct ast_node_t* ast_parse_string_literal(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
);
static struct ast_node_t* ast_parse_char_literal(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
);
static struct ast_node_t* ast_parse_integer_literal(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
);
static struct ast_node_t* ast_parse_object(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
);
static struct ast_node_t* ast_parse_object_result(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
);
static struct ast_node_t* ast_parse_object_property(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
);
static struct ast_node_t* ast_parse_decorator(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
);

// identifier ::= [A-Za-z][A-Za-z0-9_]*
static struct ast_node_t* ast_parse_identifier(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
) {
    struct token_t const* const token = (*parse_context->tokens)[*token_index];
    if (token->type == TOKEN_IDENTIFIER) {
        *token_index += 1;
        struct ast_node_t* const node =
            ast_node_create(parse_context, NODE_TYPE_IDENTIFIER, token);
        node->data.identifier.content = token_buffer(token);
        assert(str_length(node->data.identifier.content) != 0);
        return node;
    } else {
        return nullptr;
    }
}

// property ::= identifier assignment object_result ";"
static struct ast_node_t* ast_parse_property(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
) {
    struct ast_node_t* const identifier =
        ast_parse_identifier(parse_context, token_index);
    if (!identifier) {
        return nullptr;
    }

    struct token_t const* const assign_token =
        ast_eat_token(parse_context, token_index, TOKEN_ASSIGN);

    struct ast_node_t* const node =
        ast_node_create(parse_context, NODE_TYPE_PROPERTY, assign_token);
    node->data.property.identifier = identifier;
    node->data.property.object_result =
        ast_parse_object_result(parse_context, token_index);

    ast_eat_token(parse_context, token_index, TOKEN_SEMICOLON);

    return node;
}

// object_result ::= object | STRING | CHAR | INTEGER | object_property
static struct ast_node_t* ast_parse_object_result(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
) {
    struct ast_node_t* const object =
        ast_parse_object(parse_context, token_index);
    if (object) {
        return object;
    }

    struct ast_node_t* const object_property =
        ast_parse_object_property(parse_context, token_index);
    if (object_property) {
        return object_property;
    }

    struct ast_node_t* const string_literal =
        ast_parse_string_literal(parse_context, token_index);
    if (string_literal) {
        return string_literal;
    }

    struct ast_node_t* const char_literal =
        ast_parse_char_literal(parse_context, token_index);
    if (char_literal) {
        return char_literal;
    }

    struct ast_node_t* const integer =
        ast_parse_integer_literal(parse_context, token_index);
    if (integer) {
        return integer;
    }

    return nullptr;
}

static struct ast_node_t* ast_parse_string_literal(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
) {
    struct token_t const* const token = (*parse_context->tokens)[*token_index];
    if (token->type == TOKEN_STRING_LITERAL) {
        *token_index += 1;
        struct ast_node_t* const node =
            ast_node_create(parse_context, NODE_TYPE_STRING_LITERAL, token);
        node->data.string_literal.content = token_buffer(token);
        return node;
    } else {
        return nullptr;
    }
}

static struct ast_node_t* ast_parse_char_literal(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
) {
    struct token_t const* const token = (*parse_context->tokens)[*token_index];
    if (token->type == TOKEN_CHAR_LITERAL) {
        *token_index += 1;
        struct ast_node_t* const node =
            ast_node_create(parse_context, NODE_TYPE_CHAR_LITERAL, token);
        node->data.char_literal.c = token->data.char_literal.c;
        return node;
    } else {
        return nullptr;
    }
}

static struct ast_node_t* ast_parse_integer_literal(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
) {
    struct token_t const* const token = (*parse_context->tokens)[*token_index];
    if (token->type == TOKEN_INT) {
        *token_index += 1;
        struct ast_node_t* const node =
            ast_node_create(parse_context, NODE_TYPE_INTEGER, token);
        node->data.integer.content = token->data.integer.integer;
        return node;
    } else {
        return nullptr;
    }
}

// object_property ::= identifier ("(" object_result ("," object_result)+ ")")*
// ("." object_property)?
static struct ast_node_t* ast_parse_object_property(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
) {
    size_t const original_token_index = *token_index;
    struct ast_node_t* const identifier =
        ast_parse_identifier(parse_context, token_index);
    if (!identifier) {
        return nullptr;
    }

    struct ast_node_t* node = identifier;

    struct token_t const* next_token = (*parse_context->tokens)[*token_index];

    while (next_token->type == TOKEN_OPEN_PAREN) {
        *token_index += 1;
        struct ast_node_t* const free_copy = ast_node_create(
            parse_context, NODE_TYPE_FREE_OBJECT_COPY, next_token
        );
        free_copy->data.free_object_copy.property_list =
            array(struct ast_node_t*, parse_context->allocator);
        free_copy->data.free_object_copy.object = node;
        while (true) {
            struct ast_node_t* const object_result =
                ast_parse_object_result(parse_context, token_index);
            if (object_result) {
                array_push(
                    free_copy->data.free_object_copy.property_list,
                    object_result
                );
            } else {
                *token_index = original_token_index;
                return nullptr;
            }

            next_token = (*parse_context->tokens)[*token_index];
            if (next_token->type == TOKEN_CLOSE_PAREN) {
                *token_index += 1;
                next_token = (*parse_context->tokens)[*token_index];
                node       = free_copy;
                break;
            } else if (next_token->type == TOKEN_COMMA) {
                *token_index += 1;
            } else {
                *token_index = original_token_index;
                return nullptr;
            }
        }
    }

    if (next_token->type == TOKEN_DOT) {
        *token_index += 1;
        struct ast_node_t* const object_property =
            ast_parse_object_property(parse_context, token_index);
        if (object_property) {
            struct ast_node_t* const n = ast_node_create(
                parse_context, NODE_TYPE_OBJECT_PROPERTY_ACCESS, next_token
            );
            n->data.object_property_access.object   = node;
            n->data.object_property_access.property = object_property;
            return n;
        } else {
            *token_index = original_token_index;
            return nullptr;
        }
    }

    return node;
}

// object ::= (identifier+ ">")? "{" (decorator | property)* "}"
static struct ast_node_t* ast_parse_object(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
) {
    struct token_t const* token = (*parse_context->tokens)[*token_index];

    struct ast_node_t* const object =
        ast_node_create(parse_context, NODE_TYPE_OBJECT, token);
    object->data.object.free_list =
        array(struct ast_node_t*, parse_context->allocator);
    object->data.object.property_list =
        array(struct ast_node_t*, parse_context->allocator);

    size_t const original_token_index = *token_index;

    if (token->type == TOKEN_IDENTIFIER) {
        while (true) {
            struct ast_node_t* const identifier =
                ast_parse_identifier(parse_context, token_index);
            if (identifier) {
                array_push(object->data.object.free_list, identifier);
                continue;
            }
            struct token_t const* const next_token =
                (*parse_context->tokens)[*token_index];
            if (next_token->type == TOKEN_GREATER_THAN) {
                *token_index += 1;
                token = (*parse_context->tokens)[*token_index];
                break;
            } else {
                *token_index = original_token_index;
                return nullptr;
            }
        }
    }

    if (token->type == TOKEN_OPEN_BRACE) {
        *token_index += 1;
    } else {
        *token_index = original_token_index;
        return nullptr;
    }

    while (true) {
        struct ast_node_t* const decorator =
            ast_parse_decorator(parse_context, token_index);
        if (decorator) {
            array_push(object->data.object.property_list, decorator);
            continue;
        }

        struct ast_node_t* const property =
            ast_parse_property(parse_context, token_index);
        if (property) {
            array_push(object->data.object.property_list, property);
            continue;
        }

        token = (*parse_context->tokens)[*token_index];
        if (token->type == TOKEN_CLOSE_BRACE) {
            *token_index += 1;
            return object;
        } else {
            *token_index = original_token_index;
            return nullptr;
        }
    }

    vix_unreachable();
}

// decorator :== "..." object_result ";"
static struct ast_node_t* ast_parse_decorator(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
) {
    struct token_t const* const token = (*parse_context->tokens)[*token_index];
    size_t const original_token_index = *token_index;
    if (token->type == TOKEN_DOT_DOT_DOT) {
        *token_index += 1;
        struct ast_node_t* const object_result =
            ast_parse_object_result(parse_context, token_index);
        struct ast_node_t* const node =
            ast_node_create(parse_context, NODE_TYPE_DECORATOR, token);
        if (object_result) {
            node->data.decorator.object = object_result;
        } else {
            *token_index = original_token_index;
            return nullptr;
        }

        ast_eat_token(parse_context, token_index, TOKEN_SEMICOLON);
        return node;
    } else {
        return nullptr;
    }
}

// root ::= property*
static void ast_parse_root_node(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1],
    ast_node_ptr_array_t properties[static const 1]
) {
    while (true) {
        struct ast_node_t* const property_node =
            ast_parse_property(parse_context, token_index);
        if (property_node) {
            array_push(*properties, property_node);
            continue;
        }

        return;
    }
}

static struct ast_node_t* ast_parse_root(
    struct parse_context_t const parse_context[static const 1],
    size_t token_index[static const 1]
) {
    struct ast_node_t* const node = ast_node_create(
        parse_context, NODE_TYPE_ROOT, (*parse_context->tokens)[*token_index]
    );
    node->data.root.list = array(struct ast_node_t*, parse_context->allocator);
    ast_parse_root_node(parse_context, token_index, &node->data.root.list);

    return node;
}

struct ast_node_t* ast_parse(
    struct allocator_t allocator[static const 1], char const* const source,
    token_ptr_array_t tokens[static const 1],
    struct import_table_entry_t owner[static const 1],
    enum error_color_t const error_color
) {
    struct parse_context_t parse_context = {
        .error_color = error_color,
        .source      = source,
        .owner       = owner,
        .tokens      = tokens,
        .allocator   = allocator,
    };
    size_t token_index = 0;
    parse_context.root = ast_parse_root(&parse_context, &token_index);
    return parse_context.root;
}

static void visit_field(
    struct ast_node_t* node, void (*visit)(struct ast_node_t*, void* context),
    void* context
) {
    if (node) {
        visit(node, context);
    }
}

static void visit_node_list(
    struct ast_node_t** list, void (*visit)(struct ast_node_t*, void* context),
    void* context
) {
    if (list) {
        for (size_t i = 0; i < array_header(list)->length; ++i) {
            visit(list[i], context);
        }
    }
}

void ast_visit_node_children(
    struct ast_node_t node[static const 1],
    void (*visit)(struct ast_node_t*, void* context), void* context
) {
    switch (node->type) {
    case NODE_TYPE_ROOT:
        visit_node_list(node->data.root.list, visit, context);
        break;
    case NODE_TYPE_OBJECT:
        visit_node_list(node->data.object.free_list, visit, context);
        visit_node_list(node->data.object.property_list, visit, context);
        break;
    case NODE_TYPE_PROPERTY:
        visit_field(node->data.property.identifier, visit, context);
        visit_field(node->data.property.object_result, visit, context);
        break;
    case NODE_TYPE_FREE_OBJECT_COPY:
        visit_field(node->data.free_object_copy.object, visit, context);
        visit_node_list(
            node->data.free_object_copy.property_list, visit, context
        );
        break;
    case NODE_TYPE_OBJECT_PROPERTY_ACCESS:
        visit_field(node->data.object_property_access.object, visit, context);
        visit_field(
            node->data.object_property_access.property, visit, context
        );
        break;
    case NODE_TYPE_DECORATOR:
        visit_field(node->data.decorator.object, visit, context);
        break;
    case NODE_TYPE_IDENTIFIER:
        break;
    case NODE_TYPE_STRING_LITERAL:
        break;
    case NODE_TYPE_CHAR_LITERAL:
        break;
    case NODE_TYPE_INTEGER:
        break;
    }
}
