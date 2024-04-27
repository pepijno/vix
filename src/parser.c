#include "parser.h"

#include "array.h"
#include "lexer.h"
#include "util.h"

#include <assert.h>
#include <string.h>

typedef struct {
    i64 id;
    Str source;
    AstNode* root;
    TokenPtrArray* tokens;
    ErrorColor const error_color;
    ImportTableEntry* owner;
    Allocator* allocator;
} ParseContext;

static void ast_error(
    ParseContext parse_context[static 1], Token token[static 1], Str message
) {
    ErrorMessage error = error_message_create_with_line(
        parse_context->allocator, parse_context->owner->path, token->start_line,
        token->start_column, parse_context->owner->source_code,
        parse_context->owner->line_offsets, message
    );

    print_error_message(&error, parse_context->error_color);
    exit(EXIT_FAILURE);
}

static Str token_buffer(Token token[static 1]) {
    assert(
        token->type == TOKEN_STRING_LITERAL || token->type == TOKEN_IDENTIFIER
    );
    return token->string_literal.string;
}

static void ast_expect_token(
    ParseContext parse_context[static 1], Token token[static 1], TokenType type
) {
    if (token->type == type) {
        return;
    }

    StrBuffer buffer = str_buffer_new(parse_context->allocator, 0);
    str_buffer_printf(
        &buffer, str_new("expected token '%s', found '%s'"),
        token_name(type), token_name(token->type)
    );
    ast_error(parse_context, token, str_buffer_str(&buffer));
}

static Token* ast_eat_token(
    ParseContext parse_context[static 1], i64 token_index[static 1],
    TokenType type
) {
    Token* token = (*parse_context->tokens)[*token_index];
    ast_expect_token(parse_context, token, type);
    *token_index += 1;
    return token;
}

static AstNode* ast_node_create(
    ParseContext parse_context[static 1], NodeType type,
    Token first_token[static 1]
) {
    AstNode node = {
        .id     = parse_context->id,
        .type   = type,
        .line   = first_token->start_line,
        .column = first_token->start_column,
        .owner  = parse_context->owner,
    };
    AstNode* ptr = (AstNode*) parse_context->allocator->allocate(
        sizeof(AstNode), parse_context->allocator->context
    );
    memcpy(ptr, &node, sizeof(AstNode));
    parse_context->id += 1;
    return ptr;
}

static AstNode* ast_parse_property(
    ParseContext parse_context[static 1], i64 token_index[static 1]
);
static AstNode* ast_parse_string_literal(
    ParseContext parse_context[static 1], i64 token_index[static 1]
);
static AstNode* ast_parse_char_literal(
    ParseContext parse_context[static 1], i64 token_index[static 1]
);
static AstNode* ast_parse_integer_literal(
    ParseContext parse_context[static 1], i64 token_index[static 1]
);
static AstNode* ast_parse_object(
    ParseContext parse_context[static 1], i64 token_index[static 1]
);
static AstNode* ast_parse_property_value(
    ParseContext parse_context[static 1], i64 token_index[static 1]
);
static AstNode* ast_parse_object_copy(
    ParseContext parse_context[static 1], i64 token_index[static 1]
);
static AstNode* ast_parse_decorator(
    ParseContext parse_context[static 1], i64 token_index[static 1]
);

static AstNode* ast_parse_string_literal(
    ParseContext parse_context[static 1], i64 token_index[static 1]
) {
    Token* token = (*parse_context->tokens)[*token_index];
    if (token->type == TOKEN_STRING_LITERAL) {
        *token_index += 1;
        AstNode* node =
            ast_node_create(parse_context, NODE_TYPE_STRING_LITERAL, token);
        node->string_literal.content = token_buffer(token);
        return node;
    } else {
        return nullptr;
    }
}

static AstNode* ast_parse_char_literal(
    ParseContext parse_context[static 1], i64 token_index[static 1]
) {
    Token* token = (*parse_context->tokens)[*token_index];
    if (token->type == TOKEN_CHAR_LITERAL) {
        *token_index += 1;
        AstNode* node =
            ast_node_create(parse_context, NODE_TYPE_CHAR_LITERAL, token);
        node->char_literal.c = token->char_literal.c;
        return node;
    } else {
        return nullptr;
    }
}

static AstNode* ast_parse_integer_literal(
    ParseContext parse_context[static 1], i64 token_index[static 1]
) {
    Token* token = (*parse_context->tokens)[*token_index];
    if (token->type == TOKEN_INT) {
        *token_index += 1;
        AstNode* node =
            ast_node_create(parse_context, NODE_TYPE_INTEGER, token);
        node->integer.content = token->integer.integer;
        return node;
    } else {
        return nullptr;
    }
}

// identifier ::= [A-Za-z][A-Za-z0-9_]*
static AstNode* ast_parse_identifier(
    ParseContext parse_context[static 1], i64 token_index[static 1]
) {
    Token* token = (*parse_context->tokens)[*token_index];
    if (token->type == TOKEN_IDENTIFIER) {
        *token_index += 1;
        AstNode* node =
            ast_node_create(parse_context, NODE_TYPE_IDENTIFIER, token);
        node->identifier.content = token_buffer(token);
        assert(node->identifier.content.length != 0);
        return node;
    } else {
        return nullptr;
    }
}

// property ::= identifier assignment property_value ";"
static AstNode* ast_parse_property(
    ParseContext parse_context[static 1], i64 token_index[static 1]
) {
    i64 original_token_index = *token_index;
    AstNode* identifier      = ast_parse_identifier(parse_context, token_index);
    if (!identifier) {
        *token_index = original_token_index;
        return nullptr;
    }

    Token* assign_token =
        ast_eat_token(parse_context, token_index, TOKEN_ASSIGN);

    AstNode* property_value =
        ast_parse_property_value(parse_context, token_index);
    if (property_value == nullptr) {
        *token_index = original_token_index;
        return nullptr;
    }

    AstNode* node =
        ast_node_create(parse_context, NODE_TYPE_PROPERTY, assign_token);
    node->property.identifier             = identifier;
    node->property.property_value         = property_value;
    node->property.identifier->parent     = node;
    node->property.property_value->parent = node;

    printf("index %ld\n", *token_index);
    ast_eat_token(parse_context, token_index, TOKEN_SEMICOLON);

    return node;
}

// property_value ::= object | object_copy | STRING | CHAR | INTEGER
static AstNode* ast_parse_property_value(
    ParseContext parse_context[static 1], i64 token_index[static 1]
) {
    AstNode* object = ast_parse_object(parse_context, token_index);
    if (object) {
        return object;
    }

    AstNode* object_copy = ast_parse_object_copy(parse_context, token_index);
    if (object_copy) {
        return object_copy;
    }

    AstNode* string_literal =
        ast_parse_string_literal(parse_context, token_index);
    if (string_literal) {
        return string_literal;
    }

    AstNode* char_literal = ast_parse_char_literal(parse_context, token_index);
    if (char_literal) {
        return char_literal;
    }

    AstNode* integer = ast_parse_integer_literal(parse_context, token_index);
    if (integer) {
        return integer;
    }

    return nullptr;
}

// free_object_copy_params ::= "(" property_value ("," property_value)* ")"
static AstNode* ast_parse_free_object_copy_params(
    ParseContext parse_context[static 1], i64 token_index[static 1]
) {
    i64 original_token_index = *token_index;
    Token* next_token        = (*parse_context->tokens)[*token_index];

    if (next_token->type == TOKEN_OPEN_PAREN) {
        *token_index += 1;
        next_token = (*parse_context->tokens)[*token_index];
    } else {
        *token_index = original_token_index;
        return nullptr;
    }

    AstNode* free_copy = ast_node_create(
        parse_context, NODE_TYPE_FREE_OBJECT_COPY_PARAMS, next_token
    );
    free_copy->free_object_copy_params.parameter_list =
        array(AstNode*, parse_context->allocator);
    while (true) {
        AstNode* property_value =
            ast_parse_property_value(parse_context, token_index);
        if (property_value) {
            array_push(
                free_copy->free_object_copy_params.parameter_list,
                property_value
            );
            property_value->parent = free_copy;
        } else {
            *token_index = original_token_index;
            return nullptr;
        }

        next_token = (*parse_context->tokens)[*token_index];
        if (next_token->type == TOKEN_CLOSE_PAREN) {
            *token_index += 1;
            return free_copy;
        } else if (next_token->type == TOKEN_COMMA) {
            *token_index += 1;
        } else {
            *token_index = original_token_index;
            return nullptr;
        }
    }

    return free_copy;
}

// object_copy ::= identifier (free_object_copy)* ("." object_copy)?
static AstNode* ast_parse_object_copy(
    ParseContext parse_context[static 1], i64 token_index[static 1]
) {
    i64 original_token_index = *token_index;
    AstNode* identifier      = ast_parse_identifier(parse_context, token_index);
    if (!identifier) {
        *token_index = original_token_index;
        return nullptr;
    }

    Token* next_token = (*parse_context->tokens)[*token_index];

    AstNode* object_copy =
        ast_node_create(parse_context, NODE_TYPE_OBJECT_COPY, next_token);
    object_copy->object_copy.identifier = identifier;
    identifier->parent                  = object_copy;
    object_copy->object_copy.free_object_copies =
        array(AstNode*, parse_context->allocator);

    while (true) {
        AstNode* free_object_copy =
            ast_parse_free_object_copy_params(parse_context, token_index);
        if (free_object_copy != nullptr) {
            free_object_copy->parent = object_copy;
            array_push(
                object_copy->object_copy.free_object_copies, free_object_copy
            );
        } else {
            break;
        }
    }

    next_token = (*parse_context->tokens)[*token_index];

    if (next_token->type == TOKEN_DOT) {
        *token_index += 1;
        AstNode* next_object_copy =
            ast_parse_object_copy(parse_context, token_index);
        if (next_object_copy) {
            object_copy->object_copy.object_copy = next_object_copy;
            next_object_copy->parent             = object_copy;
        } else {
            *token_index = original_token_index;
            return nullptr;
        }
    }

    return object_copy;
}

// object ::= (identifier+ ">")? (("{" (decorator | property)* "}") |
// object_copy)
static AstNode* ast_parse_object(
    ParseContext parse_context[static 1], i64 token_index[static 1]
) {
    Token* token = (*parse_context->tokens)[*token_index];

    AstNode* object = ast_node_create(parse_context, NODE_TYPE_OBJECT, token);
    object->object.free_list     = array(AstNode*, parse_context->allocator);
    object->object.property_list = array(AstNode*, parse_context->allocator);

    i64 original_token_index = *token_index;

    if (token->type == TOKEN_IDENTIFIER) {
        while (true) {
            AstNode* identifier =
                ast_parse_identifier(parse_context, token_index);
            if (identifier) {
                array_push(object->object.free_list, identifier);
                identifier->parent = object;
                continue;
            }
            Token* next_token = (*parse_context->tokens)[*token_index];
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

    token = (*parse_context->tokens)[*token_index];

    if (token->type == TOKEN_OPEN_BRACE) {
        *token_index += 1;
        while (true) {
            AstNode* decorator =
                ast_parse_decorator(parse_context, token_index);
            if (decorator) {
                array_push(object->object.property_list, decorator);
                decorator->parent = object;
                continue;
            }

            AstNode* property = ast_parse_property(parse_context, token_index);
            if (property) {
                array_push(object->object.property_list, property);
                property->parent = object;
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
    } else {
        AstNode* object_copy =
            ast_parse_object_copy(parse_context, token_index);
        if (object_copy != nullptr) {
            object->object_copy.object_copy = object_copy;
            object_copy->parent             = object;
            return object;
        } else {
            *token_index = original_token_index;
            return nullptr;
        }
    }

    vix_unreachable();
}

// decorator :== "..." property_value ";"
static AstNode* ast_parse_decorator(
    ParseContext parse_context[static 1], i64 token_index[static 1]
) {
    Token* token             = (*parse_context->tokens)[*token_index];
    i64 original_token_index = *token_index;
    if (token->type == TOKEN_DOT_DOT_DOT) {
        *token_index += 1;
        AstNode* property_value =
            ast_parse_property_value(parse_context, token_index);
        AstNode* node =
            ast_node_create(parse_context, NODE_TYPE_DECORATOR, token);
        if (property_value) {
            node->decorator.object = property_value;
            property_value->parent = node;
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
static AstNode* ast_parse_root(
    ParseContext parse_context[static 1], i64 token_index[static 1]
) {
    AstNode* node = ast_node_create(
        parse_context, NODE_TYPE_ROOT, (*parse_context->tokens)[*token_index]
    );
    node->root.list = array(AstNode*, parse_context->allocator);

    while (true) {
        AstNode* property_node = ast_parse_property(parse_context, token_index);
        if (property_node) {
            array_push(node->root.list, property_node);
            property_node->parent = node;
            continue;
        }

        break;
    }

    return node;
}

AstNode* ast_parse(
    Allocator allocator[static 1], Str source, TokenPtrArray tokens[static 1],
    ImportTableEntry owner[static 1], ErrorColor error_color
) {
    ParseContext parse_context = {
        .id          = 0,
        .error_color = error_color,
        .source      = source,
        .owner       = owner,
        .tokens      = tokens,
        .allocator   = allocator,
    };
    i64 token_index    = 0;
    parse_context.root = ast_parse_root(&parse_context, &token_index);
    return parse_context.root;
}

static void visit_field(
    AstNode* node, void (*visit)(AstNode*, void* context), void* context
) {
    if (node) {
        visit(node, context);
    }
}

static void visit_node_list(
    AstNode** list, void (*visit)(AstNode*, void* context), void* context
) {
    if (list) {
        for (i64 i = 0; i < array_header(list)->length; ++i) {
            visit(list[i], context);
        }
    }
}

void ast_visit_node_children(
    AstNode node[static 1], void (*visit)(AstNode*, void* context),
    void* context
) {
    switch (node->type) {
        case NODE_TYPE_ROOT:
            visit_node_list(node->root.list, visit, context);
            break;
        case NODE_TYPE_OBJECT:
            visit_node_list(node->object.free_list, visit, context);
            visit_node_list(node->object.property_list, visit, context);
            break;
        case NODE_TYPE_PROPERTY:
            visit_field(node->property.identifier, visit, context);
            visit_field(node->property.property_value, visit, context);
            break;
        case NODE_TYPE_FREE_OBJECT_COPY_PARAMS:
            visit_node_list(
                node->free_object_copy_params.parameter_list, visit, context
            );
            break;
        case NODE_TYPE_OBJECT_COPY:
            visit_field(node->object_copy.identifier, visit, context);
            visit_node_list(
                node->object_copy.free_object_copies, visit, context
            );
            visit_field(node->object_copy.object_copy, visit, context);
            break;
        case NODE_TYPE_DECORATOR:
            visit_field(node->decorator.object, visit, context);
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
