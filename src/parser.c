#include "parser.h"

#include "array.h"
#include "lexer.h"
#include "util.h"

#include <assert.h>
#include <string.h>

typedef struct {
    size_t id;
    str_t source;
    ast_node_t* root;
    token_ptr_array_t* tokens;
    error_color_e const error_color;
    import_table_entry_t* owner;
    allocator_t* allocator;
} parse_context_t;

static void ast_error(
    parse_context_t parse_context[static 1], token_t token[static 1],
    str_t message
) {
    error_message_t error = error_message_create_with_line(
        parse_context->allocator, parse_context->owner->path, token->start_line,
        token->start_column, parse_context->owner->source_code,
        parse_context->owner->line_offsets, message
    );

    print_error_message(&error, parse_context->error_color);
    exit(EXIT_FAILURE);
}

static str_t token_buffer(token_t token[static 1]) {
    assert(
        token->type == TOKEN_STRING_LITERAL || token->type == TOKEN_IDENTIFIER
    );
    return token->data.string_literal.string;
}

static void ast_expect_token(
    parse_context_t parse_context[static 1], token_t token[static 1],
    token_type_e type
) {
    if (token->type == type) {
        return;
    }

    str_buffer_t buffer = str_buffer_new(parse_context->allocator, 0);
    str_buffer_printf(
        &buffer, str_new("expected token '%s', found '%s'"), token_name(type),
        token_name(token->type)
    );
    ast_error(parse_context, token, str_buffer_str(&buffer));
}

static token_t* ast_eat_token(
    parse_context_t parse_context[static 1], size_t token_index[static 1],
    token_type_e type
) {
    token_t* token = (*parse_context->tokens)[*token_index];
    ast_expect_token(parse_context, token, type);
    *token_index += 1;
    return token;
}

static ast_node_t* ast_node_create(
    parse_context_t parse_context[static 1], node_type_e type,
    token_t first_token[static 1]
) {
    ast_node_t node = {
        .id     = parse_context->id,
        .type   = type,
        .line   = first_token->start_line,
        .column = first_token->start_column,
        .owner  = parse_context->owner,
    };
    ast_node_t* ptr = (ast_node_t*) parse_context->allocator->allocate(
        sizeof(ast_node_t), parse_context->allocator->context
    );
    memcpy(ptr, &node, sizeof(ast_node_t));
    parse_context->id += 1;
    return ptr;
}

static ast_node_t* ast_parse_property(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
);
static ast_node_t* ast_parse_string_literal(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
);
static ast_node_t* ast_parse_char_literal(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
);
static ast_node_t* ast_parse_integer_literal(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
);
static ast_node_t* ast_parse_object(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
);
static ast_node_t* ast_parse_property_value(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
);
static ast_node_t* ast_parse_object_copy(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
);
static ast_node_t* ast_parse_decorator(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
);

static ast_node_t* ast_parse_string_literal(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
) {
    token_t* token = (*parse_context->tokens)[*token_index];
    if (token->type == TOKEN_STRING_LITERAL) {
        *token_index += 1;
        ast_node_t* node =
            ast_node_create(parse_context, NODE_TYPE_STRING_LITERAL, token);
        node->data.string_literal.content = token_buffer(token);
        return node;
    } else {
        return nullptr;
    }
}

static ast_node_t* ast_parse_char_literal(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
) {
    token_t* token = (*parse_context->tokens)[*token_index];
    if (token->type == TOKEN_CHAR_LITERAL) {
        *token_index += 1;
        ast_node_t* node =
            ast_node_create(parse_context, NODE_TYPE_CHAR_LITERAL, token);
        node->data.char_literal.c = token->data.char_literal.c;
        return node;
    } else {
        return nullptr;
    }
}

static ast_node_t* ast_parse_integer_literal(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
) {
    token_t* token = (*parse_context->tokens)[*token_index];
    if (token->type == TOKEN_INT) {
        *token_index += 1;
        ast_node_t* node =
            ast_node_create(parse_context, NODE_TYPE_INTEGER, token);
        node->data.integer.content = token->data.integer.integer;
        return node;
    } else {
        return nullptr;
    }
}

// identifier ::= [A-Za-z][A-Za-z0-9_]*
static ast_node_t* ast_parse_identifier(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
) {
    token_t* token = (*parse_context->tokens)[*token_index];
    if (token->type == TOKEN_IDENTIFIER) {
        *token_index += 1;
        ast_node_t* node =
            ast_node_create(parse_context, NODE_TYPE_IDENTIFIER, token);
        node->data.identifier.content = token_buffer(token);
        assert(node->data.identifier.content.length != 0);
        return node;
    } else {
        return nullptr;
    }
}

// property ::= identifier assignment property_value ";"
static ast_node_t* ast_parse_property(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
) {
    size_t original_token_index = *token_index;
    ast_node_t* identifier = ast_parse_identifier(parse_context, token_index);
    if (!identifier) {
        *token_index = original_token_index;
        return nullptr;
    }

    token_t* assign_token =
        ast_eat_token(parse_context, token_index, TOKEN_ASSIGN);

    ast_node_t* property_value =
        ast_parse_property_value(parse_context, token_index);
    if (property_value == nullptr) {
        *token_index = original_token_index;
        return nullptr;
    }

    ast_node_t* node =
        ast_node_create(parse_context, NODE_TYPE_PROPERTY, assign_token);
    node->data.property.identifier             = identifier;
    node->data.property.property_value         = property_value;
    node->data.property.identifier->parent     = node;
    node->data.property.property_value->parent = node;

    printf("index %ld\n", *token_index);
    ast_eat_token(parse_context, token_index, TOKEN_SEMICOLON);

    return node;
}

// property_value ::= object | object_copy | STRING | CHAR | INTEGER
static ast_node_t* ast_parse_property_value(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
) {
    ast_node_t* object = ast_parse_object(parse_context, token_index);
    if (object) {
        return object;
    }

    ast_node_t* object_copy = ast_parse_object_copy(parse_context, token_index);
    if (object_copy) {
        return object_copy;
    }

    ast_node_t* string_literal =
        ast_parse_string_literal(parse_context, token_index);
    if (string_literal) {
        return string_literal;
    }

    ast_node_t* char_literal =
        ast_parse_char_literal(parse_context, token_index);
    if (char_literal) {
        return char_literal;
    }

    ast_node_t* integer = ast_parse_integer_literal(parse_context, token_index);
    if (integer) {
        return integer;
    }

    return nullptr;
}

// free_object_copy_params ::= "(" property_value ("," property_value)* ")"
static ast_node_t* ast_parse_free_object_copy_params(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
) {
    size_t original_token_index = *token_index;
    token_t* next_token         = (*parse_context->tokens)[*token_index];

    if (next_token->type == TOKEN_OPEN_PAREN) {
        *token_index += 1;
        next_token = (*parse_context->tokens)[*token_index];
    } else {
        *token_index = original_token_index;
        return nullptr;
    }

    ast_node_t* free_copy = ast_node_create(
        parse_context, NODE_TYPE_FREE_OBJECT_COPY_PARAMS, next_token
    );
    free_copy->data.free_object_copy_params.parameter_list =
        array(ast_node_t*, parse_context->allocator);
    while (true) {
        ast_node_t* property_value =
            ast_parse_property_value(parse_context, token_index);
        if (property_value) {
            array_push(
                free_copy->data.free_object_copy_params.parameter_list,
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
static ast_node_t* ast_parse_object_copy(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
) {
    size_t original_token_index = *token_index;
    ast_node_t* identifier = ast_parse_identifier(parse_context, token_index);
    if (!identifier) {
        *token_index = original_token_index;
        return nullptr;
    }

    token_t* next_token = (*parse_context->tokens)[*token_index];

    ast_node_t* object_copy =
        ast_node_create(parse_context, NODE_TYPE_OBJECT_COPY, next_token);
    object_copy->data.object_copy.identifier = identifier;
    identifier->parent                       = object_copy;
    object_copy->data.object_copy.free_object_copies =
        array(ast_node_t*, parse_context->allocator);

    while (true) {
        ast_node_t* free_object_copy =
            ast_parse_free_object_copy_params(parse_context, token_index);
        if (free_object_copy != nullptr) {
            free_object_copy->parent = object_copy;
            array_push(
                object_copy->data.object_copy.free_object_copies,
                free_object_copy
            );
        } else {
            break;
        }
    }

    next_token = (*parse_context->tokens)[*token_index];

    if (next_token->type == TOKEN_DOT) {
        *token_index += 1;
        ast_node_t* next_object_copy =
            ast_parse_object_copy(parse_context, token_index);
        if (next_object_copy) {
            object_copy->data.object_copy.object_copy = next_object_copy;
            next_object_copy->parent                  = object_copy;
        } else {
            *token_index = original_token_index;
            return nullptr;
        }
    }

    return object_copy;
}

// object ::= (identifier+ ">")? (("{" (decorator | property)* "}") |
// object_copy)
static ast_node_t* ast_parse_object(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
) {
    token_t* token = (*parse_context->tokens)[*token_index];

    ast_node_t* object =
        ast_node_create(parse_context, NODE_TYPE_OBJECT, token);
    object->data.object.free_list =
        array(ast_node_t*, parse_context->allocator);
    object->data.object.property_list =
        array(ast_node_t*, parse_context->allocator);

    size_t original_token_index = *token_index;

    if (token->type == TOKEN_IDENTIFIER) {
        while (true) {
            ast_node_t* identifier =
                ast_parse_identifier(parse_context, token_index);
            if (identifier) {
                array_push(object->data.object.free_list, identifier);
                identifier->parent = object;
                continue;
            }
            token_t* next_token = (*parse_context->tokens)[*token_index];
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
            ast_node_t* decorator =
                ast_parse_decorator(parse_context, token_index);
            if (decorator) {
                array_push(object->data.object.property_list, decorator);
                decorator->parent = object;
                continue;
            }

            ast_node_t* property =
                ast_parse_property(parse_context, token_index);
            if (property) {
                array_push(object->data.object.property_list, property);
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
        ast_node_t* object_copy =
            ast_parse_object_copy(parse_context, token_index);
        if (object_copy != nullptr) {
            object->data.object_copy.object_copy = object_copy;
            object_copy->parent                  = object;
            return object;
        } else {
            *token_index = original_token_index;
            return nullptr;
        }
    }

    vix_unreachable();
}

// decorator :== "..." property_value ";"
static ast_node_t* ast_parse_decorator(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
) {
    token_t* token              = (*parse_context->tokens)[*token_index];
    size_t original_token_index = *token_index;
    if (token->type == TOKEN_DOT_DOT_DOT) {
        *token_index += 1;
        ast_node_t* property_value =
            ast_parse_property_value(parse_context, token_index);
        ast_node_t* node =
            ast_node_create(parse_context, NODE_TYPE_DECORATOR, token);
        if (property_value) {
            node->data.decorator.object = property_value;
            property_value->parent      = node;
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
static ast_node_t* ast_parse_root(
    parse_context_t parse_context[static 1], size_t token_index[static 1]
) {
    ast_node_t* node = ast_node_create(
        parse_context, NODE_TYPE_ROOT, (*parse_context->tokens)[*token_index]
    );
    node->data.root.list = array(ast_node_t*, parse_context->allocator);

    while (true) {
        ast_node_t* property_node =
            ast_parse_property(parse_context, token_index);
        if (property_node) {
            array_push(node->data.root.list, property_node);
            property_node->parent = node;
            continue;
        }

        break;
    }

    return node;
}

ast_node_t* ast_parse(
    allocator_t allocator[static 1], str_t source,
    token_ptr_array_t tokens[static 1], import_table_entry_t owner[static 1],
    error_color_e error_color
) {
    parse_context_t parse_context = {
        .id          = 0,
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
    ast_node_t* node, void (*visit)(ast_node_t*, void* context), void* context
) {
    if (node) {
        visit(node, context);
    }
}

static void visit_node_list(
    ast_node_t** list, void (*visit)(ast_node_t*, void* context), void* context
) {
    if (list) {
        for (size_t i = 0; i < array_header(list)->length; ++i) {
            visit(list[i], context);
        }
    }
}

void ast_visit_node_children(
    ast_node_t node[static 1], void (*visit)(ast_node_t*, void* context),
    void* context
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
            visit_field(node->data.property.property_value, visit, context);
            break;
        case NODE_TYPE_FREE_OBJECT_COPY_PARAMS:
            visit_node_list(
                node->data.free_object_copy_params.parameter_list, visit,
                context
            );
            break;
        case NODE_TYPE_OBJECT_COPY:
            visit_field(node->data.object_copy.identifier, visit, context);
            visit_node_list(
                node->data.object_copy.free_object_copies, visit, context
            );
            visit_field(node->data.object_copy.object_copy, visit, context);
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
