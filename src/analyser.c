#include "analyser.h"

#include "array.h"
#include "error_message.h"
#include "parser.h"
#include "str.h"
#include "util.h"

static struct error_message_t* add_node_error(
    struct allocator_t allocator[static const 1],
    struct code_gen_t code_gen[static const 1],
    struct ast_node_t const node[static const 1], char const* const message
) {
    struct error_message_t error_message = error_message_create_with_line(
        allocator, node->owner->path, node->line, node->column,
        node->owner->source_code, node->owner->line_offsets, message
    );

    struct error_message_t* err =
        allocator->allocate(sizeof(struct error_message_t), allocator->context);
    memcpy(err, &error_message, sizeof(struct error_message_t));

    array_push(code_gen->errors, err);

    return err;
}

static void add_error_note(
    struct allocator_t allocator[static const 1],
    struct error_message_t parent_message[static const 1],
    struct ast_node_t const node[static const 1], char const* const message
) {
    struct error_message_t error_message = error_message_create_with_line(
        allocator, node->owner->path, node->line, node->column,
        node->owner->source_code, node->owner->line_offsets, message
    );

    struct error_message_t* err =
        allocator->allocate(sizeof(struct error_message_t), allocator->context);
    memcpy(err, &error_message, sizeof(struct error_message_t));

    error_message_add_note(parent_message, err);
}

static struct ast_node_t const* find_identifier(
    struct scope_t const scope[static const 1], char const* const identifier
) {
    for (size_t i = 0; i < array_length_unsigned(scope->properties); ++i) {
        if (str_equal(
                scope->properties[i]->data.identifier.content, identifier
            )) {
            return scope->properties[i];
        }
    }
    return nullptr;
}

static void analyse_node(
    struct allocator_t allocator[static const 1],
    struct code_gen_t code_gen[static const 1],
    struct scope_t scope[static const 1],
    struct ast_node_t const node[static const 1]
);

static void analyse_property(
    struct allocator_t allocator[static const 1],
    struct code_gen_t code_gen[static const 1],
    struct scope_t scope[static const 1],
    struct ast_node_t const property[static const 1]
);

static void analyse_identifier(
    struct allocator_t allocator[static const 1],
    struct code_gen_t code_gen[static const 1],
    struct scope_t scope[static const 1],
    struct ast_node_t const identifier[static const 1]
) {
    char const* const identifier_content = identifier->data.identifier.content;
    struct ast_node_t const* existing_identifier =
        find_identifier(scope, identifier_content);
    if (existing_identifier) {
        struct error_message_t* error_message = add_node_error(
            allocator, code_gen, identifier,
            str_printf(allocator, "redefinition of '%s'", identifier_content)
        );
        add_error_note(
            allocator, error_message, existing_identifier,
            str_printf(allocator, "previous definition is here")
        );
    } else {
        array_push(scope->properties, identifier);
    }
}

static void analyse_root(
    struct allocator_t allocator[static const 1],
    struct code_gen_t code_gen[static const 1],
    struct scope_t scope[static const 1],
    struct ast_node_t const root[static const 1]
) {
    for (size_t i = 0; i < array_length_unsigned(root->data.root.list); ++i) {
        analyse_node(allocator, code_gen, scope, root->data.root.list[i]);
    }
}

static void analyse_object(
    struct allocator_t allocator[static const 1],
    struct code_gen_t code_gen[static const 1],
    struct scope_t scope[static const 1],
    struct ast_node_t const object[static const 1]
) {
    struct scope_t new_scope = {
        .parent      = scope,
        .source_node = object,
    };
    new_scope.properties = array(struct ast_node_t*, allocator);

    for (size_t i = 0; i < array_length_unsigned(object->data.object.free_list);
         ++i) {
        analyse_identifier(
            allocator, code_gen, &new_scope, object->data.object.free_list[i]
        );
    }
    for (size_t i = 0;
         i < array_length_unsigned(object->data.object.property_list); ++i) {
        struct ast_node_t* node = object->data.object.property_list[i];
        if (node->type == NODE_TYPE_PROPERTY) {
            analyse_property(
                allocator, code_gen, &new_scope,
                object->data.object.property_list[i]
            );
        } else if (node->type == NODE_TYPE_DECORATOR) {
        } else {
            vix_unreachable();
        }
    }
}

static void analyse_property(
    struct allocator_t allocator[static const 1],
    struct code_gen_t code_gen[static const 1],
    struct scope_t scope[static const 1],
    struct ast_node_t const property[static const 1]
) {
    assert(
        str_length(property->data.property.identifier->data.identifier.content
        ) != 0
    );
    analyse_identifier(
        allocator, code_gen, scope, property->data.property.identifier
    );
    analyse_node(
        allocator, code_gen, scope, property->data.property.object_result
    );
}

static void analyse_node(
    struct allocator_t allocator[static const 1],
    struct code_gen_t code_gen[static const 1],
    struct scope_t scope[static const 1],
    struct ast_node_t const node[static const 1]
) {
    switch (node->type) {
    case NODE_TYPE_ROOT:
        analyse_root(allocator, code_gen, scope, node);
        break;
    case NODE_TYPE_PROPERTY:
        analyse_property(allocator, code_gen, scope, node);
        break;
    case NODE_TYPE_OBJECT:
        analyse_object(allocator, code_gen, scope, node);
    case NODE_TYPE_STRING_LITERAL:
    case NODE_TYPE_CHAR_LITERAL:
    case NODE_TYPE_INTEGER:
        break;
    }
}

static void report_errors_and_maybe_exit(
    struct code_gen_t const code_gen[static const 1]
) {
    if (array_length_unsigned(code_gen->errors) > 0) {
        for (size_t i = 0; i < array_length_unsigned(code_gen->errors); ++i) {
            struct error_message_t* error_message = code_gen->errors[i];
            print_error_message(error_message, code_gen->error_color);
        }
        exit(1);
    }
}

void analyse(
    struct allocator_t allocator[static const 1],
    struct ast_node_t const root[static const 1]
) {
    struct code_gen_t code_gen = {
        .error_color = ERROR_COLOR_ON,
        .root_scope =
            {
                         .parent      = nullptr,
                         .source_node = root,
                         },
    };
    code_gen.errors                = array(struct error_message_t*, allocator);
    code_gen.root_scope.properties = array(struct ast_node_t*, allocator);

    analyse_node(allocator, &code_gen, &code_gen.root_scope, root);

    report_errors_and_maybe_exit(&code_gen);
}
